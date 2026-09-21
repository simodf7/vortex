module circular_queue #(
    parameter integer DEPTH = 64,
    parameter integer WIDTH = 7
)(
    input  wire             aclk,
    input  wire             aresetn,

    // Producer side (Buffer Manager) 
    input  wire             push,
    input  wire [WIDTH-1:0] data_in,
    output wire             full,

    input  wire             pop,
    output wire [WIDTH-1:0] data_out,

    output wire             empty
); 

		localparam integer PTR_W = (DEPTH <= 1) ? 1 : $clog2(DEPTH); 
	

 		reg [WIDTH-1:0] elem [0:DEPTH-1];
    reg [PTR_W-1:0] head_queue;
    reg [PTR_W-1:0] tail_queue;
    reg [PTR_W:0]   count_queue;

	  assign full   = (count_queue == DEPTH);
    assign empty  = (count_queue == 0);
	 

		wire do_push = push && !full; 
		wire do_pop  = pop  && !empty;  	

	  assign data_out = elem[head_queue]; 


		always @(posedge aclk) begin 
			if(!aresetn) begin 
					  tail_queue   <= {PTR_W{1'b0}}; 
            head_queue   <= {PTR_W{1'b0}}; 
            count_queue  <= {PTR_W + 1{1'b0}}; 
			end else begin 

				if(do_push) begin 	 
						elem[tail_queue] <= data_in; 
						tail_queue <= tail_queue + 1'b1;
				end 	
		
				if(do_pop) begin 
						head_queue <= head_queue + 1'b1; 
				end 	

				case ({do_push, do_pop}) 
						2'b10: count_queue <= count_queue + 1'b1; 
						2'b01: count_queue <= count_queue - 1'b1; 
						default: count_queue <= count_queue; 
			  endcase 

		 end 
		end 


endmodule 
