`default_nettype none

module trng_top(
    input  wire clk_50mhz,
    input  wire rst_btn,
    output wire led_d3,
    output wire led_d4,
    output wire led_d5,
    output wire led_d6,
    output wire led_d8,
    output wire led_d9,
    output wire led_d10,
    output wire led_d11
);

    wire [15:0] rand_out;

    trng u_trng (
        .clk(clk_50mhz),
        .rst(rst_btn),
        .rand_out(rand_out)
    );

    assign led_d3  = rand_out[0];
    assign led_d4  = rand_out[1];
    assign led_d5  = rand_out[2];
    assign led_d6  = rand_out[3];
    assign led_d8  = rand_out[8];
    assign led_d9  = rand_out[9];
    assign led_d10 = rand_out[10];
    assign led_d11 = rand_out[11];

endmodule

`default_nettype wire