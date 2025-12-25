`timescale 1ns / 1ps
`include "ring_oscillator.v"
`include "trng.v"
`include "spu_core.v"

module tb_spu_core;
    reg clk, rst_n;
    // Dummy inputs for physics
    reg signed [15:0] h, j0, j1, j2, j3, temp;
    reg signed [1:0] s0, s1, s2, s3;
    wire signed [1:0] out_spin;

    spu_core uut (
        .clk(clk), .rst_n(rst_n),
        .h_local_field(h),
        .j_coupling_0(j0), .j_coupling_1(j1), .j_coupling_2(j2), .j_coupling_3(j3),
        .neighbor_spin_0(s0), .neighbor_spin_1(s1), .neighbor_spin_2(s2), .neighbor_spin_3(s3),
        .temperature(temp),
        .current_spin_state(out_spin)
    );

    initial begin
        clk=0; 
        forever #5 clk=~clk;
    end

    initial begin
        $dumpfile("spu_core_test.vcd");
        $dumpvars(0, tb_spu_core);
        
        rst_n = 0; #20; rst_n = 1;

        // Set parameters to encourage flipping
        h = 0; 
        j0=0; j1=0; j2=0; j3=0;
        s0=1; s1=1; s2=1; s3=1;
        temp = 16'd1000; // High temp = high randomness

        #5000;
        $finish;
    end
endmodule