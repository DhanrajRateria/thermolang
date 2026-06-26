`default_nettype none

module uart_tx #(
    parameter integer CLK_HZ = 50000000,
    parameter integer BAUD   = 9600
)(
    input  wire       clk,
    input  wire       rst,
    input  wire [7:0] data_in,
    input  wire       start,
    output reg        tx,
    output reg        busy
);

    localparam integer CLKS_PER_BIT = CLK_HZ / BAUD;
    localparam integer CTR_W = 16;

    reg [9:0] shift_reg;
    reg [3:0] bit_idx;
    reg [CTR_W-1:0] clk_count;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            tx        <= 1'b1;
            busy      <= 1'b0;
            shift_reg <= 10'h3FF;
            bit_idx   <= 4'd0;
            clk_count <= {CTR_W{1'b0}};
        end else begin
            if (!busy) begin
                tx <= 1'b1;
                if (start) begin
                    // frame = start(0), data[7:0], stop(1)
                    shift_reg <= {1'b1, data_in, 1'b0};
                    busy      <= 1'b1;
                    bit_idx   <= 4'd0;
                    clk_count <= {CTR_W{1'b0}};
                    tx        <= 1'b0; // start bit immediately
                end
            end else begin
                if (clk_count == CLKS_PER_BIT - 1) begin
                    clk_count <= {CTR_W{1'b0}};
                    bit_idx   <= bit_idx + 1'b1;
                    shift_reg <= {1'b1, shift_reg[9:1]};
                    tx        <= shift_reg[1];

                    if (bit_idx == 4'd9) begin
                        busy <= 1'b0;
                        tx   <= 1'b1;
                    end
                end else begin
                    clk_count <= clk_count + 1'b1;
                end
            end
        end
    end

endmodule

`default_nettype wire