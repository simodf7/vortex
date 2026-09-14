`timescale 1ns / 1ps

module buffer_manager #(
	 	parameter integer NUM_HARTS = 64, 
		parameter integer NUM_BUFFERS = NUM_HARTS, 
		parameter integer LINE_LOG2 = 7, // 2^7 = 128 byte per buffer 
		parameter integer INDEX_WIDTH = $clog2(NUM_BUFFERS)
)(
		input wire 								   aclk, 
	  input wire 									 aresetn,


		// AXI frontend 
    input wire [INDEX_WIDTH-1:0] wr_index,
    input wire [7:0]             wr_char,
    input wire                   wr_valid,
    output wire                  wr_ready, 


		// Formatter
		output wire 								  flush_valid, 
		output wire [INDEX_WIDTH-1:0] flush_index,
    output wire [LINE_LOG2:0] 	  flush_len, 
		input  wire [LINE_LOG2-1:0]	  rd_addr, 
		output reg  [7:0]						  rd_data, 
		input  wire 								  flush_done

);
 
	localparam integer LINE_SIZE = (1 << LINE_LOG2);

	
	// queue signals 
	
  wire [INDEX_WIDTH-1:0] q_head; 
	wire q_full; 
	wire q_empty; 
	reg  q_push; 
  reg  q_pop; 
	reg  [INDEX_WIDTH-1:0] q_elem_push;  		
	

  // Circular queue
  // here are stored index of the buffers ready to flush 


	circular_queue 
			#( 
				.DEPTH(NUM_BUFFERS), 
				.WIDTH(INDEX_WIDTH)	
	 ) queue (
    	.aclk (aclk),
    	.aresetn (aresetn),
			.push (q_push), 
			.data_in (q_elem_push), 
			.full (q_full), 
			.pop (q_pop), 
			.data_out (q_head), 
			.empty (q_empty) 
		);
	

	// BUFFERS
  
	// Buffer
	reg [7:0] buffer [0:NUM_BUFFERS-1][0:LINE_SIZE-1]; 
	
	// Wptr[x] is the pointer to the next element for the 2^x-th buffer 
  reg [LINE_LOG2:0] wptr [0:NUM_BUFFERS-1]; 

	// a bit for each buffer tells if it's ready to bu flushed 
	reg buffer_status [0:NUM_BUFFERS-1]; 

  
	// Condition to flush 

	// 1) Buffer is full -> there is space only for the last character 
	wire buffer_full = (wptr[wr_index] == LINE_SIZE -1);	 
	
	// 2) A newline was sent 
	wire is_newline  = (wr_char == 8'h0A); 


	
	wire handshake_done = wr_valid && wr_ready; 
	wire flush_cond = handshake_done && (buffer_full || is_newline); 
	wire do_store = handshake_done && !is_newline; 
	


	// Buffer manager is ready when 
	// 1) the transaction is not related to a buffer whose flush is pending 
	// 2) the queue is not full (the char incoming could trigger the flush condition) 
	assign wr_ready = !buffer_status[wr_index] && !q_full; 


	assign flush_valid = !q_empty; 
	assign flush_index = q_head; 
	assign flush_len = wptr[q_head];
  assign q_push = flush_cond; 
  assign q_elem_push = wr_index; 
  assign q_pop = flush_done; 

	
	// Read and Write on Buffers 
	always @(posedge aclk) begin
			if(do_store) begin 
					buffer[wr_index][wptr[wr_index]] <= wr_char; 
			end 
			rd_data <= buffer[q_head][rd_addr];  
	end 

	integer i; 
  
	always @(posedge aclk) begin 
			if(!aresetn) begin 
				 for(i = 0; i < NUM_BUFFERS; i = i+1) begin 
						wptr[i] <= 0; 
						buffer_status[i] <= 1'b0; 
				 end 
			end 
			else begin 
				
					if(do_store) begin 
						 wptr[wr_index] <= wptr[wr_index] + 1; 
					end 

					if(flush_cond) begin
							buffer_status[wr_index] <= 1'b1; 
					end 

					if(flush_done) begin
							wptr[q_head] <= 0; 
						  buffer_status[q_head] <= 1'b0;  
					end 	
			
			end


	end 



endmodule 
