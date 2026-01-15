`timescale 1ns / 1ps

module ring_oscillator #(
    parameter DELAY = 1 // Configurable delay to force jitter
)(
    input wire enable,
    output wire osc_out
);
    // (* DONT_TOUCH = "TRUE" *) attributes are for Vivado synthesis.
    // For iverilog simulation, the #DELAY prevents zero-delay loops.
    
    wire w1, w2, w3;

    // NAND Gate with delay (Control mechanism)
    nand #DELAY gate1 (w1, enable, w3);

    // Inverter Chain
    not #DELAY gate2 (w2, w1);
    not #DELAY gate3 (w3, w2);

    assign osc_out = w3;

endmodule