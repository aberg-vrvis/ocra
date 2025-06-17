// async_fifo_tb.v

`timescale 1ns/1ps

module async_fifo_tb;

    localparam DATA_WIDTH = 16;
    localparam ADDR_WIDTH = 4;

    reg wr_clk = 0;
    reg wr_rst_n = 0;
    reg wr_en = 0;
    reg [DATA_WIDTH-1:0] wr_data = 0;
    wire full;
    wire almost_full;

    reg rd_clk = 0;
    reg rd_rst_n = 0;
    wire [DATA_WIDTH-1:0] rd_data;
    wire rd_valid;
    reg rd_en = 0;
    wire empty;
    wire almost_empty;

    async_fifo #(
        .DATA_WIDTH(DATA_WIDTH),
        .ADDR_WIDTH(ADDR_WIDTH)
    ) dut (
        .wr_clk(wr_clk),
        .wr_rst_n(wr_rst_n),
        .wr_data(wr_data),
        .wr_en(wr_en),
        .full(full),
        .almost_full(almost_full),

        .rd_clk(rd_clk),
        .rd_rst_n(rd_rst_n),
        .rd_data(rd_data),
        .rd_valid(rd_valid),
        .rd_en(rd_en),
        .empty(empty),
        .almost_empty(almost_empty)
    );

    // Write clock: 10ns period (100 MHz)
    always #5 wr_clk = ~wr_clk;

    // Read clock: 12ns period (~83.3 MHz)
    always #6 rd_clk = ~rd_clk;

    initial begin
        $dumpfile("fifo_tb.vcd");
        $dumpvars(0, async_fifo_tb);

        #0  wr_rst_n = 0; rd_rst_n = 0;
        #20 wr_rst_n = 1; rd_rst_n = 1;

        // Write 20 values into FIFO
        repeat (20) begin
            @(posedge wr_clk);
            if (!full) begin
                wr_en <= 1;
                wr_data <= wr_data + 1;
            end else begin
                wr_en <= 0;
            end
        end
        wr_en <= 0;

        // Wait a few cycles
        repeat (10) @(posedge wr_clk);

        // Begin reading
        repeat (25) begin
            @(posedge rd_clk);
            rd_en <= ~empty;
        end
        rd_en <= 0;

        // End simulation
        repeat (10) @(posedge rd_clk);
        $finish;
    end
endmodule

