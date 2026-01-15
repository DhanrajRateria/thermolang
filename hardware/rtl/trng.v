`timescale 1ns / 1ps

module trng (
    input wire clk,
    input wire rst,
    output reg [15:0] rand_out
);
    wire ro1, ro2, ro3, ro4;

    // Use PRIME NUMBER delays to simulate physical chaos
    ring_oscillator #(.DELAY(3)) ro1_inst (.enable(!rst), .osc_out(ro1));
    ring_oscillator #(.DELAY(5)) ro2_inst (.enable(!rst), .osc_out(ro2));
    ring_oscillator #(.DELAY(7)) ro3_inst (.enable(!rst), .osc_out(ro3));
    ring_oscillator #(.DELAY(11)) ro4_inst (.enable(!rst), .osc_out(ro4));

    // Mix the entropy
    wire raw_entropy = ro1 ^ ro2 ^ ro3 ^ ro4;

    // Sample into shift register on System Clock
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            rand_out <= 16'b0;
        end else begin
            rand_out <= {rand_out[14:0], raw_entropy};
        end
    end

endmodule