`timescale 1ns / 1ps

module vx_console #(
    parameter integer CLK_FREQ_HZ = 100_000_000, // frequenza di aclk
    parameter integer BAUD_RATE   = 115_200,
    parameter integer AXI_ADDR_W  = 12,
    parameter [63:0]  COUT_BASE_ADDR = 64'hA0120000,
    parameter integer NUM_HARTS   = 64,          // numero di thread globali
    parameter integer LINE_LOG2   = 7            // 2^7 = 128 byte per linea
)(
    input  wire                    aclk,
    input  wire                    aresetn,

    // ------------------- AXI4-Lite slave -------------------
    input  wire [AXI_ADDR_W-1:0]   s_axi_awaddr,
    input  wire [2:0]              s_axi_awprot,
    input  wire                    s_axi_awvalid,
    output wire                    s_axi_awready,

    input  wire [31:0]             s_axi_wdata,
    input  wire [3:0]              s_axi_wstrb,    
    input  wire                    s_axi_wvalid,
    output wire                    s_axi_wready,

    output wire [1:0]              s_axi_bresp,
    output reg                     s_axi_bvalid,
    input  wire                    s_axi_bready,

    input  wire [AXI_ADDR_W-1:0]   s_axi_araddr,
    input  wire [2:0]              s_axi_arprot,
    input  wire                    s_axi_arvalid,
    output wire                    s_axi_arready,

    output wire [31:0]             s_axi_rdata,
    output wire [1:0]              s_axi_rresp,
    output reg                     s_axi_rvalid,
    input  wire                    s_axi_rready,

    // ------------------- UART TX ---------------------------
    output wire                    uart_tx, 
    input wire                     uart_rx  // Added just to infer UART interface
);

    localparam integer LINE_SIZE = (1 << LINE_LOG2);
    localparam integer HART_LOG2 = 6;                       // 64 hart

    reg [7:0] buffer [0:NUM_HARTS-1][0:LINE_SIZE-1]; // buffer
    reg [LINE_LOG2:0] wptr [0:NUM_HARTS-1];

    reg [NUM_HARTS-1:0] hart_ready;
    reg [HART_LOG2-1:0] buffer_ready_queue [0:NUM_HARTS-1];
    reg [HART_LOG2-1:0] head_queue;
    reg [HART_LOG2-1:0] tail_queue;
    reg [HART_LOG2:0]   count_queue;

    wire full_queue  = (count_queue == NUM_HARTS);
    wire empty_queue = (count_queue == 0);  

    // Coda Circolare
    // Quando un hart invia il carattere \n o non c'è più spazio, allora
    // il buffer corrispondente viene considerato Ready e può essere
    // elaborato dal Formatter che poi procederà a inviarlo alla UART

    // Codificatore
    //
    // AXI Smartconnect deve tradurre da AXI-full a 512 bit ad AXI-lite a 32
    // bit. Essendo di questi 64 byte (512 bit) solo uno scritto, solo un bit
    // di 64 del WSTRB sarà asserito e lo smartconnect genererà una sola
    // transazione in scrittura a 32 bit, a un indirizzo allineato alla word
    // di 32 bit. Es. se viene scritto il byte nella posizione 0x50, la
    // transazione AXI full continuerà a essere relativa all'indirizzo 0x40 ma
    // lo smartconnect genererà la transizione all'indirizzo 0x48 (indirizzo
    // di partenza della terza word da 32 bit) con WSTRB = 0100.
    //
    // se WSTRB = 0001 (primo byte) -> 00
    // se WSTRB = 0010 (secondo byte) -> 01
    // se WSTRB = 0100 (terzo byte) -> 10
    // se WSTRB = 1000 (quarto byte) -> 11

    reg [1:0] wstrb_offset;             

    always @(*) begin
        case (s_axi_wstrb)
            4'b0001: wstrb_offset = 2'b00;
            4'b0010: wstrb_offset = 2'b01;
            4'b0100: wstrb_offset = 2'b10;
            4'b1000: wstrb_offset = 2'b11;
            default: wstrb_offset = 2'b00;
        endcase
    end

    wire [HART_LOG2-1:0] hart_index = {s_axi_awaddr[5:2], wstrb_offset};

    // check per controllare se la transazione ha almeno un bit alto nello
    // strobe
    wire has_strobe = |s_axi_wstrb;

    // IMPORTANTE: l'IP è mappato da A012_0000 fino ad A012_0080 (quindi 128
    // byte) ma in realtà secondo la configurazione attuale di Vortex possiamo
    // avere fino a 64 indirizzi diversi associati ai vari hart quindi
    // l'ultimo indirizzo consentito è A012_0040, tuttavia Vivado consente
    // come dimensione minima di un segmento di memoria 128 byte. 
    // Di conseguenza bisogna gestire eventuali transazioni che vanno da 0x40
    // fino a 0x80. Consideriamo una transazione valida solo se rientra in
    // quel range, per verificarlo basta guardare al 7 bit. 
    wire in_region  = !s_axi_awaddr[6];  
    wire valid_char = has_strobe && in_region;

    wire [7:0] cur_char   = s_axi_wdata[wstrb_offset*8 +: 8];
    wire       is_newline = (cur_char == 8'h0A);

    // una risposta pendente (bvalid) blocca comunque tutto
    assign s_axi_awready = !s_axi_bvalid && !(valid_char && hart_ready[hart_index]) && !full_queue;
    assign s_axi_wready  = s_axi_awready;
    assign s_axi_bresp   = 2'b00;

    wire w_en     = s_axi_awvalid && s_axi_wvalid && s_axi_awready;
    wire buffer_full = (wptr[hart_index] == LINE_SIZE-1);  
    wire flush_cond = is_newline || buffer_full; 
    wire do_flush = w_en && valid_char && flush_cond;

    integer i;
    always @(posedge aclk) begin                      
        if (!aresetn) begin
            s_axi_bvalid <= 1'b0;
            hart_ready   <= {NUM_HARTS{1'b0}};
            tail_queue   <= {HART_LOG2{1'b0}};
            head_queue   <= {HART_LOG2{1'b0}};
            count_queue  <= {(HART_LOG2+1){1'b0}};
            for (i = 0; i < NUM_HARTS; i = i + 1)
                wptr[i] <= {(LINE_LOG2+1){1'b0}};
        end else begin
            if (w_en)  // gestisce le transazioni nel range vuoto 
                s_axi_bvalid <= 1'b1;
            else if (s_axi_bvalid && s_axi_bready)
                s_axi_bvalid <= 1'b0;

            if (w_en && valid_char) begin
		if(flush_cond) begin   // Newline oppure ultimo carattere 
			if(wptr[hart_index] == (LINE_SIZE-1)) begin
		    	   buffer[hart_index][wptr[hart_index][LINE_LOG2-1:0]] <= cur_char; 
			   wptr[hart_index] <= wptr[hart_index] + 1'b1;    
		   	end 

			hart_ready[hart_index] <= 1'b1; 
			buffer_ready_queue[tail_queue] <= hart_index; 
			tail_queue <= tail_queue + 1'b1; 
		end 
		else begin 
		    buffer[hart_index][wptr[hart_index][LINE_LOG2-1:0]] <= cur_char; 
                    wptr[hart_index] <= wptr[hart_index] + 1'b1;
                end
            end

            if (end_buffer_tx) begin
                hart_ready[tid] <= 1'b0;
                wptr[tid]       <= {(LINE_LOG2+1){1'b0}};
                head_queue      <= head_queue + 1'b1;
            end

	    // Se usiamo due if, il secondo sovrascriverebbe il primo 
            case ({do_flush, end_buffer_tx})
                2'b10:   count_queue <= count_queue + 1'b1;
                2'b01:   count_queue <= count_queue - 1'b1;
                default: count_queue <= count_queue;
            endcase
        end
    end

    // ------------------- Canale di lettura AXI  -------------
    assign s_axi_arready = ~s_axi_rvalid;
    assign s_axi_rdata   = 32'h0;
    assign s_axi_rresp   = 2'b00;

    always @(posedge aclk) begin
        if (!aresetn)
            s_axi_rvalid <= 1'b0;
        else if (s_axi_arvalid && s_axi_arready)
            s_axi_rvalid <= 1'b1;
        else if (s_axi_rvalid && s_axi_rready)
            s_axi_rvalid <= 1'b0;
    end

    // Formatter

    reg end_buffer_tx;
    reg [LINE_LOG2:0] bptr;                            
    wire [HART_LOG2-1:0] tid = buffer_ready_queue[head_queue];  
    wire [3:0] decimals_tid = tid / 4'd10;
    wire [3:0] units_tid    = tid % 4'd10;            

    localparam IDLE   = 2'b00,                         
               PREFIX = 2'b01,
               BODY   = 2'b10,
               CRLF   = 2'b11;

    reg [1:0] current_state;

    reg [2:0] char_id;
    reg [7:0] prefix_char;
    always @(*) begin
        case (char_id)
            3'd0:    prefix_char = "T";
            3'd1:    prefix_char = 8'h30 + {4'b0000, decimals_tid};
            3'd2:    prefix_char = 8'h30 + {4'b0000, units_tid};
            3'd3:    prefix_char = ":"; 
	    3'd4:    prefix_char = " "; 	    
            default: prefix_char = " ";
        endcase
    end

    reg [7:0] crlf_char;
    always @(*) begin
        case (char_id)
            3'd0:    crlf_char = 8'h0D; // "\r"
            3'd1:    crlf_char = 8'h0A; // "\n"
            default: crlf_char = " ";
        endcase
    end

    wire [7:0] tx_data = (current_state == PREFIX) ? prefix_char :
                         (current_state == BODY)   ? buffer[tid][bptr[LINE_LOG2-1:0]] :
                         crlf_char;                    

    wire tx_valid = (current_state == PREFIX) ||
                    (current_state == BODY)   ||
                    (current_state == CRLF);
    

    always @(posedge aclk) begin                       
        if (!aresetn) begin
            current_state <= IDLE;
            end_buffer_tx <= 1'b0;
            char_id       <= 3'd0;
            bptr          <= {(LINE_LOG2+1){1'b0}};
        end else begin
            end_buffer_tx <= 1'b0; 

            case (current_state)
                IDLE: begin
                    char_id <= 3'd0;
                    bptr    <= {(LINE_LOG2+1){1'b0}};
                    if (!empty_queue) current_state <= PREFIX;
                end
                PREFIX: begin
                    if (tx_ready) begin
                        if (char_id == 3'd4) begin
			   // Se arriva un newline ma non c'è niente nel buffer 	
			   if (wptr[tid] == 0) begin 
                                end_buffer_tx <= 1'b1;
                                current_state <= CRLF;
                            end 
			    else begin
                                current_state <= BODY;
                            end
                            char_id <= 0;
                        end else begin
                            char_id <= char_id + 1'b1;
                        end
                    end
                end
                BODY: begin
                    if (tx_ready) begin
                        if (bptr == wptr[tid] - 1'b1) begin
                            end_buffer_tx <= 1'b1;
                            current_state <= CRLF;
                        end else begin
                            bptr <= bptr + 1'b1;
                        end                           
                    end
                end
                CRLF: begin
                    if (tx_ready) begin
                        if (char_id == 3'd1) begin
                            current_state <= IDLE;
                            char_id <= 0;
                        end else begin
                            char_id <= char_id + 1'b1;
                        end
                    end
                end
            endcase
        end
    end

    // UART Transmitter

    reg  busy;
    wire tx_ready = (uart_state == U_IDLE);  
    reg  tx;
    assign uart_tx = tx;

    localparam B_LIMIT = (CLK_FREQ_HZ / BAUD_RATE);    

    // Stati della macchina (FSM) 
    localparam U_IDLE  = 2'b00;
    localparam U_START = 2'b01;
    localparam U_DATA  = 2'b10;
    localparam U_STOP  = 2'b11;

    reg [1:0]  uart_state;
    reg [15:0] baud_count;
    reg [2:0]  bit_index;
    reg [7:0]  shift_reg;

    always @(posedge aclk) begin                       
        if (!aresetn) begin
            tx <= 1'b1;
            busy <= 1'b0;
            uart_state <= U_IDLE;
            baud_count <= 16'b0;
            bit_index <= 3'b0;
            shift_reg <= 8'b0;
        end else begin
            case (uart_state)                        
                U_IDLE: begin
                    tx <= 1'b1;
                    busy <= 1'b0;
                    if (tx_valid) begin
                        busy <= 1'b1;
                        shift_reg <= tx_data;
                        uart_state <= U_START;
                        baud_count <= 16'b0;
                    end
                end

                U_START: begin
                    tx <= 1'b0; // Bit di start
                    if (baud_count < B_LIMIT - 1) begin
                        baud_count <= baud_count + 1'b1;
                    end else begin
                        baud_count <= 16'b0;
                        bit_index <= 3'b0;
                        uart_state <= U_DATA;
                    end
                end

                U_DATA: begin
                    tx <= shift_reg[bit_index];
                    if (baud_count < B_LIMIT - 1) begin
                        baud_count <= baud_count + 1'b1;
                    end else begin
                        baud_count <= 16'b0;
                        if (bit_index < 3'b111) begin
                            bit_index <= bit_index + 1'b1;
                        end else begin
                            uart_state <= U_STOP;
                        end
                    end
                end

                U_STOP: begin
                    tx <= 1'b1; // Bit di stop
                    if (baud_count < B_LIMIT - 1) begin
                        baud_count <= baud_count + 1'b1;
                    end else begin
                        baud_count <= 16'b0;
                        uart_state <= U_IDLE;
                    end
                end

                default: uart_state <= U_IDLE;
            endcase
        end
    end

endmodule
