`timescale 1ns / 1ps

module tb_trng();

    // Inputs to our box (Registers because we change them)
    reg clk;
    reg rst;

    // Outputs from our box (Wires because we just watch them)
    wire random_bit;

    // Plug in the TRNG module (Unit Under Test - UUT)
    trng uut (
        .clk(clk),
        .rst(rst),
        .random_bit(random_bit)
    );

    // 1. Create a Clock
    // Toggle clk every 5 nanoseconds (100 MHz speed)
    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end

    // 2. Run the Scenario
    initial begin
        // Setup file for viewing in GTKWave
        $dumpfile("trng_waves.vcd");
        $dumpvars(0, tb_trng);

        // Start with Reset ON
        rst = 1;
        #20; // Wait 20 nanoseconds

        // Release Reset (Let the oscillators run!)
        rst = 0;
        
        // Let it run for 1000 nanoseconds
        #1000;

        // Stop simulation
        $finish;
    end

    // 3. Print results to console
    always @(posedge clk) begin
        if (!rst) $display("Time: %t | Random Bit: %b", $time, random_bit);
    end

endmodule