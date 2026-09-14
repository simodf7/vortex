`timescale 1ns / 1ps

module uart_tx #(
    parameter integer CLK_FREQ_HZ = 100_000_000,
    parameter integer BAUD_RATE   = 115_200
)(
    input  wire       aclk,
    input  wire       aresetn,

    input  wire [7:0] tx_data,
    input  wire       tx_valid,
    output wire       tx_ready,

    output reg        uart_tx_o
);

    localparam integer B_LIMIT = (CLK_FREQ_HZ / BAUD_RATE);
    localparam integer CNT_W   = $clog2(B_LIMIT);

    localparam [1:0] U_IDLE  = 2'b00,
                     U_START = 2'b01,
                     U_DATA  = 2'b10,
                     U_STOP  = 2'b11;

    reg [1:0]       state;
    reg [CNT_W-1:0] baud_count;
    reg [2:0]       bit_index;
    reg [7:0]       shift_reg;

    assign tx_ready = (state == U_IDLE);

    wire tick = (baud_count == B_LIMIT-1);

    always @(posedge aclk) begin
        if (!aresetn) begin
            uart_tx_o  <= 1'b1;
            state      <= U_IDLE;
            baud_count <= {CNT_W{1'b0}};
            bit_index  <= 3'd0;
            shift_reg  <= 8'h00;
        end else begin
            case (state)
                U_IDLE: begin
                    uart_tx_o  <= 1'b1;
                    baud_count <= {CNT_W{1'b0}};
                    if (tx_valid) begin
                        shift_reg <= tx_data;
                        state     <= U_START;
                    end
                end

                U_START: begin
                    uart_tx_o <= 1'b0;
                    if (tick) begin
                        baud_count <= {CNT_W{1'b0}};
                        bit_index  <= 3'd0;
                        state      <= U_DATA;
                    end else begin
                        baud_count <= baud_count + 1'b1;
                    end
                end

                U_DATA: begin
                    uart_tx_o <= shift_reg[bit_index];
                    if (tick) begin
                        baud_count <= {CNT_W{1'b0}};
                        if (bit_index == 3'd7)
                            state <= U_STOP;
                        else
                            bit_index <= bit_index + 1'b1;
                    end else begin
                        baud_count <= baud_count + 1'b1;
                    end
                end

                U_STOP: begin
                    uart_tx_o <= 1'b1;
                    if (tick) begin
                        baud_count <= {CNT_W{1'b0}};
                        state      <= U_IDLE;
                    end else begin
                        baud_count <= baud_count + 1'b1;
                    end
                end

                default: state <= U_IDLE;
            endcase
        end
    end

endmodule
