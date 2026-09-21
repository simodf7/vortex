`timescale 1ns / 1ps
 
module axi_frontend #(
    parameter integer AXI_ADDR_W = 7,
	 	parameter integer NUM_HARTS = 64, 
		parameter integer NUM_BUFFERS = NUM_HARTS, 
		parameter integer INDEX_WIDTH = $clog2(NUM_BUFFERS)
)(
		input wire 								   aclk, 
	  input wire 									 aresetn,

    // Address write channel
    input  wire [AXI_ADDR_W-1:0] s_axi_awaddr,
    input  wire                  s_axi_awvalid,
    output wire                  s_axi_awready,


		// Data write channel 
    input  wire [31:0]           s_axi_wdata,
    input  wire [3:0]            s_axi_wstrb,
    input  wire                  s_axi_wvalid,
    output wire                  s_axi_wready,
	
		// Write response channel 
		output wire [1:0] 					 s_axi_bresp, 
		output wire 								 s_axi_bvalid, 
		input  wire 								 s_axi_bready,
	

		// Read address channel 
		input  wire [AXI_ADDR_W-1:0]  s_axi_araddr, 
		input  wire 									s_axi_arvalid, 
		output wire 									s_axi_arready,	


		// Read data channel 
		output wire [31:0]  					s_axi_rdata, 
		output wire [1:0] 						s_axi_rresp, 
		output wire 									s_axi_rvalid, 
		input  wire 									s_axi_rready, 

		// TO Buffer manager 
    output wire [INDEX_WIDTH-1:0] wr_index,
    output wire [7:0]             wr_char,
    output reg                    wr_valid,
    input  wire                   wr_ready 
		
);
	

	// Internal State 

	reg [INDEX_WIDTH-1:0] reg_index;
	reg [7:0]             reg_char;
	reg [AXI_ADDR_W-1:0]  awaddr_reg;
	reg [31:0]            wdata_reg;
	reg [3:0]             wstrb_reg;
	reg                   awaddr_rec, wdata_rec;
	reg                   bvalid_reg;
	reg [1:0]             bresp_reg;


	assign wr_index = reg_index; 
	assign wr_char = reg_char;   

	// Codificatore
    
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
        case (wstrb_reg)
            4'b0001: wstrb_offset = 2'b00;
            4'b0010: wstrb_offset = 2'b01;
            4'b0100: wstrb_offset = 2'b10;
            4'b1000: wstrb_offset = 2'b11;
            default: wstrb_offset = 2'b00;
        endcase
  end

  wire [INDEX_WIDTH-1:0] hart_index = {awaddr_reg[2 +: (INDEX_WIDTH - 2)], wstrb_offset};
	wire [7:0] 						 char = wdata_reg[8 * wstrb_offset +: 8]; 
	
	// Transaction is relative to an existing buffer 
	wire is_valid = hart_index < NUM_BUFFERS && |wstrb_reg; 

	// We received address and data 
	wire data_rec = awaddr_rec && wdata_rec; 

  // Commit event is when we received data and: 
  // -1) transaction is not valid 
	// -2) transaction is valid and there is no handshake pending (so reg index and char are available) 
	wire commit = data_rec && (!wr_valid || !is_valid);  
	wire handshake_done = wr_valid && wr_ready; 

	// we accept the next axi transaction if there's 'not one still to commit a
	assign s_axi_awready = !awaddr_rec && !bvalid_reg; 
	assign s_axi_wready = !wdata_rec && !bvalid_reg;
 
  assign s_axi_bvalid = bvalid_reg; 
  assign s_axi_bresp = bresp_reg; 

	always @(posedge aclk) begin 
		if(!aresetn) begin 
			wr_valid   <= 1'b0; 
			reg_index  <= 0; 
			reg_char   <= 0;
			awaddr_reg <= 0; 
			wdata_reg  <= 0; 
			wstrb_reg  <= 0; 
			awaddr_rec <= 1'b0;  		
			wdata_rec  <= 1'b0;  		
			bvalid_reg <= 1'b0; 
			bresp_reg  <= 2'b00; 
		end 
		else begin 
			if(s_axi_awready && s_axi_awvalid) begin 
					awaddr_reg <= s_axi_awaddr; 
					awaddr_rec <= 1'b1; 
			end else if(commit) awaddr_rec <= 1'b0;   
		
			if(s_axi_wready && s_axi_wvalid) begin 
					wdata_reg <= s_axi_wdata;
					wstrb_reg <= s_axi_wstrb;  
					wdata_rec <= 1'b1; 
			end else if(commit) wdata_rec <= 1'b0;  

			if(commit && is_valid) begin 
					reg_index <= hart_index; 
					reg_char <= char; 
					wr_valid <= 1'b1; 	
			end else if(handshake_done) wr_valid <= 1'b0; 	

			if(commit) begin 
					bvalid_reg <=	1'b1; 
					bresp_reg  <= 2'b00; 
			end else if(s_axi_bready && bvalid_reg) bvalid_reg <= 1'b0;  
	 end 
	
	
end 	




	// Read Channel (This is a Write only peripheral so we don't matter) 
	reg        rvalid_reg;
	reg [31:0] rdata_reg;
	reg [1:0]  rresp_reg;	


	assign s_axi_arready = !rvalid_reg;
	assign s_axi_rvalid  = rvalid_reg;
	assign s_axi_rdata   = rdata_reg;
	assign s_axi_rresp   = rresp_reg;	

	always @(posedge aclk) begin
			if (!aresetn) begin
					rvalid_reg <= 1'b0;
					rdata_reg  <= 32'b0;
					rresp_reg  <= 2'b00;
			end
			else begin
					if (s_axi_arvalid && s_axi_arready) begin
							rdata_reg  <= 32'b0;
							rresp_reg  <= 2'b00;
							rvalid_reg <= 1'b1;
					end

					if (rvalid_reg && s_axi_rready) begin
							rvalid_reg <= 1'b0;
					end
			end
	end

endmodule; 
