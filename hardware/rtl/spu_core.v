`default_nettype none

module spu_core #(
    parameter DATA_WIDTH     = 16,
    parameter FRAC_BITS      = 12,
    parameter NEIGHBOR_COUNT = 4,
    parameter SIM_SEED       = 16'hACE1
)(
    input  wire                          clk,
    input  wire                          rst_n,
    input  wire                          update_enable,
    input  wire signed [DATA_WIDTH-1:0]  h_local_field,

    input  wire signed [DATA_WIDTH-1:0]  j_coupling_0,
    input  wire signed [DATA_WIDTH-1:0]  j_coupling_1,
    input  wire signed [DATA_WIDTH-1:0]  j_coupling_2,
    input  wire signed [DATA_WIDTH-1:0]  j_coupling_3,

    input  wire signed [1:0]             neighbor_spin_0,
    input  wire signed [1:0]             neighbor_spin_1,
    input  wire signed [1:0]             neighbor_spin_2,
    input  wire signed [1:0]             neighbor_spin_3,

    input  wire        [DATA_WIDTH-1:0]  temperature,

    output reg  signed [1:0]             current_spin_state
);

    localparam ACC_WIDTH = DATA_WIDTH + 4;

    // ------------------------------------------------------------
    // Entropy source:
    // - hardware: real TRNG
    // - simulation: per-cell LFSR with unique seed
    // ------------------------------------------------------------
`ifdef SIMULATION
    reg [15:0] lfsr_state;
    wire [15:0] internal_rand_num = lfsr_state;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            lfsr_state <= (SIM_SEED == 16'h0000) ? 16'h0001 : SIM_SEED;
        end else begin
            lfsr_state <= {
                lfsr_state[14:0],
                lfsr_state[15] ^ lfsr_state[13] ^ lfsr_state[12] ^ lfsr_state[10]
            };
        end
    end
`else
    wire [15:0] internal_rand_num;

    trng entropy_source (
        .clk(clk),
        .rst(!rst_n),
        .rand_out(internal_rand_num)
    );
`endif

    // Neighbor products
    wire signed [DATA_WIDTH:0] prod_0 = j_coupling_0 * neighbor_spin_0;
    wire signed [DATA_WIDTH:0] prod_1 = j_coupling_1 * neighbor_spin_1;
    wire signed [DATA_WIDTH:0] prod_2 = j_coupling_2 * neighbor_spin_2;
    wire signed [DATA_WIDTH:0] prod_3 = j_coupling_3 * neighbor_spin_3;

    reg signed [ACC_WIDTH-1:0] local_field_sum;
    reg signed [ACC_WIDTH-1:0] delta_energy;
    reg signed [ACC_WIDTH-1:0] abs_delta_energy;
    reg [ACC_WIDTH-1:0] denom;
    reg [15:0] acceptance_threshold;
    reg accept_flip;

    // local_field_sum = h + sum(J_ij * s_j)
    always @(*) begin
        local_field_sum =
            $signed(h_local_field) +
            $signed(prod_0) +
            $signed(prod_1) +
            $signed(prod_2) +
            $signed(prod_3);
    end

    // ΔE = 2 * s_i * (h_i + sum(J_ij s_j))
    always @(*) begin
        delta_energy = ($signed(local_field_sum) <<< 1) * $signed(current_spin_state);
    end

    // Simple bounded stochastic acceptance
    always @(*) begin
        accept_flip = 1'b0;
        acceptance_threshold = 16'd0;
        abs_delta_energy = delta_energy[ACC_WIDTH-1] ? -delta_energy : delta_energy;
        denom = (temperature == 0) ? 1 : { {(ACC_WIDTH-DATA_WIDTH){1'b0}}, temperature };

        if ($signed(delta_energy) <= 0) begin
            accept_flip = 1'b1;
            acceptance_threshold = 16'hFFFF;
        end else begin
            acceptance_threshold = 16'hFFFF / (1 + (abs_delta_energy / denom));
            accept_flip = (internal_rand_num < acceptance_threshold);
        end
    end

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            current_spin_state <= 2'sd1; // initialize to +1
        end else if (update_enable && accept_flip) begin
            current_spin_state <= -current_spin_state;
        end
    end

endmodule

`default_nettype wire