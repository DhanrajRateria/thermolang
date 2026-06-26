`timescale 1ns / 1ps
`default_nettype none

module trng (
    input  wire        clk,
    input  wire        rst,
    output reg [15:0]  rand_out
);

    // Preserve RO outputs
    (* KEEP = "TRUE", DONT_TOUCH = "TRUE" *) wire ro1;
    (* KEEP = "TRUE", DONT_TOUCH = "TRUE" *) wire ro2;
    (* KEEP = "TRUE", DONT_TOUCH = "TRUE" *) wire ro3;
    (* KEEP = "TRUE", DONT_TOUCH = "TRUE" *) wire ro4;

    // Preserve instances
    (* DONT_TOUCH = "TRUE" *)
    ring_oscillator #(.DELAY(3)) ro1_inst (
        .enable(!rst),
        .osc_out(ro1)
    );

    (* DONT_TOUCH = "TRUE" *)
    ring_oscillator #(.DELAY(5)) ro2_inst (
        .enable(!rst),
        .osc_out(ro2)
    );

    (* DONT_TOUCH = "TRUE" *)
    ring_oscillator #(.DELAY(7)) ro3_inst (
        .enable(!rst),
        .osc_out(ro3)
    );

    (* DONT_TOUCH = "TRUE" *)
    ring_oscillator #(.DELAY(11)) ro4_inst (
        .enable(!rst),
        .osc_out(ro4)
    );

    // Entropy mixing
    (* KEEP = "TRUE", DONT_TOUCH = "TRUE" *) wire raw_entropy;
    assign raw_entropy = ro1 ^ ro2 ^ ro3 ^ ro4;

    // Sample mixed entropy into a shift register
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            rand_out <= 16'h0000;
        end else begin
            rand_out <= {rand_out[14:0], raw_entropy};
        end
    end

endmodule

`default_nettype wire