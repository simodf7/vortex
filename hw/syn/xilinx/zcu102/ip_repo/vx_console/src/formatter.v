`timescale 1ns / 1ps

module formatter #(
		parameter integer NUM_HARTS = 64, 
		parameter integer NUM_BUFFERS = NUM_HARTS,
		parameter integer INDEX_WIDTH = $clog2(NUM_BUFFERS), 
    parameter integer LINE_LOG2  = 7            // 2^7 = 128 byte per linea
)(
    input  wire             aclk,
    input  wire             aresetn,
		
		// Buffer Manager 		
		input  wire										flush_valid, 
		input  wire [INDEX_WIDTH-1:0] flush_index, 		
		input  wire [LINE_LOG2:0]			flush_len, 
		
		output wire [LINE_LOG2-1:0]		rd_addr, 
		input  wire [7:0]							rd_data, 
		output wire										flush_done, 

		// Uart tx
		input  wire										tx_ready, 
		output wire										tx_valid, 
		output wire [7:0]							tx_data 
);

    reg [LINE_LOG2:0] num_chars;                
		reg [INDEX_WIDTH-1:0] buffer_id;              


		// Formatter FSM
    
		localparam IDLE   = 2'b00,                         
               PREFIX = 2'b01,
               BODY   = 2'b10,
               CRLF   = 2'b11;

    reg [1:0] current_state;




    // Prefix format 
    wire [3:0] decimals_tid = buffer_id / 4'd10;
    wire [3:0] units_tid    = buffer_id % 4'd10;            
    
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


		// Carriage Return and Line Feed 

    reg [7:0] crlf_char;
    always @(*) begin
        case (char_id)
            3'd0:    crlf_char = 8'h0D; // "\r"
            3'd1:    crlf_char = 8'h0A; // "\n"
            default: crlf_char = " ";
        endcase
    end

		assign tx_valid = (current_state == PREFIX) || (current_state == BODY) || (current_state == CRLF); 

		reg [7:0] char; 
		assign tx_data = char; 
		
		always @(*) begin 
				case(current_state) 
						PREFIX: char = prefix_char; 
						BODY:   char = rd_data; 
						CRLF:   char = crlf_char; 
						default: char = " ";
				endcase
	  end 	
	
	  reg [LINE_LOG2:0] bptr; 
		wire tx_fire = tx_valid && tx_ready; 
		
		assign rd_addr = (current_state == BODY && tx_fire) ? bptr + 1'b1 :
                 		 (current_state == BODY)            ? bptr        :
                                                      		0;

		assign flush_done = (current_state == CRLF) && (char_id == 3'd1) && tx_fire; 

    always @(posedge aclk) begin                       
        if (!aresetn) begin
            char_id <= 0; 
						bptr <= 0;
						num_chars <= 0; 
						buffer_id <= 0;  
						current_state <= IDLE;
 	      end else begin
						case (current_state) 
								
								IDLE: begin 
										char_id <= 3'd0; 
										bptr <= 0; 
										if(flush_valid) begin 
												num_chars <= flush_len; 
												buffer_id <= flush_index; 
												current_state <= PREFIX; 
										end 
								end 

							  PREFIX: begin 
										if(tx_fire) begin
												if(char_id == 3'd4) begin 
													if(num_chars == 0) current_state <= CRLF;  
													else current_state <= BODY; 
													char_id <= 3'd0; 	
												end else char_id <= char_id + 1; 
										end 
							  end 		


								BODY: begin
										if(tx_fire) begin 
											 if(bptr == num_chars-1) current_state <= CRLF; 
											 else bptr <= bptr + 1; 
										end  	
								end 



								CRLF: begin 
										if(tx_fire) begin 
												if(char_id == 3'd1) begin 
													current_state <= IDLE; 
													char_id <= 3'd0; 
												end  
												else char_id <= char_id + 1; 
										end 
								end 
				
								default: current_state <= IDLE; 
						endcase 
				end 
		end 


			
endmodule 
