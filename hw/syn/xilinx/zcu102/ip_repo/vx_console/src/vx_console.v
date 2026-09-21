`timescale 1ns / 1ps

module vx_console #(
    parameter integer CLK_FREQ_HZ = 100_000_000, // frequenza di aclk
    parameter integer BAUD_RATE   = 115_200,
    parameter integer AXI_ADDR_W  = 7,           // 128 byte = minimo Vivado
    parameter integer NUM_HARTS   = 64,          // numero di thread globali
    parameter integer LINE_LOG2   = 7,           // 2^7 = 128 byte per linea
    parameter integer NUM_BUFFERS = NUM_HARTS ,
    parameter integer INDEX_WIDTH = $clog2(NUM_BUFFERS)
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
    output wire                    s_axi_bvalid,
    input  wire                    s_axi_bready,

    input  wire [AXI_ADDR_W-1:0]   s_axi_araddr,
    input  wire [2:0]              s_axi_arprot,
    input  wire                    s_axi_arvalid,
    output wire                    s_axi_arready,

    output wire [31:0]             s_axi_rdata,
    output wire [1:0]              s_axi_rresp,
    output wire                    s_axi_rvalid,
    input  wire                    s_axi_rready,

    // ------------------- UART ------------------------------
    output wire                    uart_tx
);

    // frontend -> buffer_manager
    wire [INDEX_WIDTH-1:0] wr_index;
    wire [7:0]             wr_char;
    wire                   wr_valid;
    wire                   wr_ready;

    // buffer_manager <-> formatter
    wire                   flush_valid;
    wire [INDEX_WIDTH-1:0] flush_index;
    wire [LINE_LOG2:0]     flush_len;
    wire [LINE_LOG2-1:0]   rd_addr;
    wire [7:0]             rd_data;
    wire                   flush_done;

    // formatter -> uart_tx
    wire [7:0]             tx_data;
    wire                   tx_valid;
    wire                   tx_ready;

    axi_frontend #(
        .AXI_ADDR_W  (AXI_ADDR_W),
        .NUM_HARTS   (NUM_HARTS),
        .NUM_BUFFERS (NUM_BUFFERS),
        .INDEX_WIDTH (INDEX_WIDTH)
    ) u_frontend (
        .aclk          (aclk),
        .aresetn       (aresetn),
        .s_axi_awaddr  (s_axi_awaddr),
        .s_axi_awvalid (s_axi_awvalid),
        .s_axi_awready (s_axi_awready),
        .s_axi_wdata   (s_axi_wdata),
        .s_axi_wstrb   (s_axi_wstrb),
        .s_axi_wvalid  (s_axi_wvalid),
        .s_axi_wready  (s_axi_wready),
        .s_axi_bresp   (s_axi_bresp),
        .s_axi_bvalid  (s_axi_bvalid),
        .s_axi_bready  (s_axi_bready),
        .s_axi_araddr  (s_axi_araddr),
        .s_axi_arvalid (s_axi_arvalid),
        .s_axi_arready (s_axi_arready),
        .s_axi_rdata   (s_axi_rdata),
        .s_axi_rresp   (s_axi_rresp),
        .s_axi_rvalid  (s_axi_rvalid),
        .s_axi_rready  (s_axi_rready),
        .wr_index      (wr_index),
        .wr_char       (wr_char),
        .wr_valid      (wr_valid),
        .wr_ready      (wr_ready)
    );

    buffer_manager #(
        .NUM_HARTS   (NUM_HARTS),
        .NUM_BUFFERS (NUM_BUFFERS),
        .LINE_LOG2   (LINE_LOG2),
        .INDEX_WIDTH (INDEX_WIDTH)
    ) u_buffers (
        .aclk        (aclk),
        .aresetn     (aresetn),
        .wr_index    (wr_index),
        .wr_char     (wr_char),
        .wr_valid    (wr_valid),
        .wr_ready    (wr_ready),
        .flush_valid (flush_valid),
        .flush_index (flush_index),
        .flush_len   (flush_len),
        .rd_addr     (rd_addr),
        .rd_data     (rd_data),
        .flush_done  (flush_done)
    );

    formatter #(
        .NUM_HARTS   (NUM_HARTS),
        .NUM_BUFFERS (NUM_BUFFERS),
        .INDEX_WIDTH (INDEX_WIDTH),
        .LINE_LOG2   (LINE_LOG2)
    ) u_formatter (
        .aclk        (aclk),
        .aresetn     (aresetn),
        .flush_valid (flush_valid),
        .flush_index (flush_index),
        .flush_len   (flush_len),
        .rd_addr     (rd_addr),
        .rd_data     (rd_data),
        .flush_done  (flush_done),
        .tx_ready    (tx_ready),
        .tx_valid    (tx_valid),
        .tx_data     (tx_data)
    );

    uart_tx #(
        .CLK_FREQ_HZ (CLK_FREQ_HZ),
        .BAUD_RATE   (BAUD_RATE)
    ) u_uart (
        .aclk      (aclk),
        .aresetn   (aresetn),
        .tx_data   (tx_data),
        .tx_valid  (tx_valid),
        .tx_ready  (tx_ready),
        .uart_tx_o (uart_tx)
    );

endmodule
