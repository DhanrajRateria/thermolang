`timescale 1ns / 1ps
`default_nettype none

module tb_trng;

    reg clk;
    reg rst;
    wire [15:0] rand_out;

    trng uut (
        .clk(clk),
        .rst(rst),
        .rand_out(rand_out)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    initial begin
        $dumpfile("trng_waves.vcd");
        $dumpvars(0, tb_trng);

        rst = 1'b1;
        #20;
        rst = 1'b0;

        #2000;
        $finish;
    end

    always @(posedge clk) begin
        if (!rst) begin
            $display("Time: %0t | rand_out: %h", $time, rand_out);
        end
    end

endmodule

`default_nettype wire