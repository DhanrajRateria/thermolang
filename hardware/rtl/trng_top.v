`default_nettype none

module trng_top(
    input  wire clk,
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

    reg [31:0] heartbeat_ctr;
    reg [23:0] sample_div;
    reg [15:0] sampled_rand;

    trng u_trng (
        .clk(clk),
        .rst(rst_btn),
        .rand_out(rand_out)
    );

    always @(posedge clk or posedge rst_btn) begin
        if (rst_btn) begin
            heartbeat_ctr <= 32'd0;
            sample_div    <= 24'd0;
            sampled_rand  <= 16'd0;
        end else begin
            heartbeat_ctr <= heartbeat_ctr + 1'b1;
            sample_div    <= sample_div + 1'b1;

            if (sample_div == 24'd0)
                sampled_rand <= rand_out;
        end
    end

    // One heartbeat LED proves the design is alive
    assign led_d3  = heartbeat_ctr[25];

    // Seven visible random sample bits
    assign led_d4  = sampled_rand[0];
    assign led_d5  = sampled_rand[1];
    assign led_d6  = sampled_rand[2];
    assign led_d8  = sampled_rand[3];
    assign led_d9  = sampled_rand[4];
    assign led_d10 = sampled_rand[5];
    assign led_d11 = sampled_rand[6];

endmodule

`default_nettype wire