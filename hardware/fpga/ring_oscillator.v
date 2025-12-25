`timescale 1ns / 1ps

module ring_oscillator #(
    parameter DELAY = 1 // Default delay is 1ns
)(
    input wire enable,
    output wire osc_out
);

    wire w1, w2, w3;

    // The NAND gate enables/disables the loop
    nand #DELAY gate1 (w1, enable, w3);

    // Inverters with configurable delay
    // This simulates manufacturing differences
    not #DELAY gate2 (w2, w1);
    not #DELAY gate3 (w3, w2);

    assign osc_out = w3;

endmodule