`timescale 1ns / 1ps

module trng (
    input wire clk,
    input wire rst,
    output reg [15:0] rand_out
);

    wire ro1_out, ro2_out, ro3_out, ro4_out;

    // Instantiate with DIFFERENT delays to force desynchronization
    // These specific numbers (2, 3, 5, 7) are prime, which helps chaos.
    ring_oscillator #(.DELAY(2)) ro1 (.enable(!rst), .osc_out(ro1_out));
    ring_oscillator #(.DELAY(3)) ro2 (.enable(!rst), .osc_out(ro2_out));
    ring_oscillator #(.DELAY(5)) ro3 (.enable(!rst), .osc_out(ro3_out));
    ring_oscillator #(.DELAY(7)) ro4 (.enable(!rst), .osc_out(ro4_out));

    wire raw_entropy;
    assign raw_entropy = ro1_out ^ ro2_out ^ ro3_out ^ ro4_out;

    // Shift Register Accumulator
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            rand_out <= 16'b0;
        end else begin
            rand_out <= {rand_out[14:0], raw_entropy};
        end
    end

endmodule