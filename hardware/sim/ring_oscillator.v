`timescale 1ns / 1ps
`default_nettype none

module ring_oscillator #(
    parameter integer DELAY = 1
)(
    input  wire enable,
    output wire osc_out
);

    // Preserve the oscillator loop in synthesis/implementation
    (* KEEP = "TRUE", DONT_TOUCH = "TRUE" *) wire w1;
    (* KEEP = "TRUE", DONT_TOUCH = "TRUE" *) wire w2;
    (* KEEP = "TRUE", DONT_TOUCH = "TRUE" *) wire w3;

    // Controlled 3-stage ring oscillator
    // In simulation, #DELAY helps avoid zero-delay loop issues.
    // In hardware, these delays are ignored; physical routing delay dominates.
    nand #DELAY gate1 (w1, enable, w3);
    not  #DELAY gate2 (w2, w1);
    not  #DELAY gate3 (w3, w2);

    // Preserve oscillator output
    (* KEEP = "TRUE", DONT_TOUCH = "TRUE" *) wire osc_int;
    assign osc_int = w3;
    assign osc_out = osc_int;

endmodule

`default_nettype wire