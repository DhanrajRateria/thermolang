`default_nettype none

module spu_core #(
    parameter DATA_WIDTH     = 16,
    parameter FRAC_BITS      = 12,
    parameter NEIGHBOR_COUNT = 4
)(
    input wire                      clk,
    input wire                      rst_n,
    input wire signed [DATA_WIDTH-1:0]  h_local_field,
    
    // Flattened array ports
    input wire signed [DATA_WIDTH-1:0]  j_coupling_0,
    input wire signed [DATA_WIDTH-1:0]  j_coupling_1,
    input wire signed [DATA_WIDTH-1:0]  j_coupling_2,
    input wire signed [DATA_WIDTH-1:0]  j_coupling_3,
    
    input wire signed [1:0]             neighbor_spin_0,
    input wire signed [1:0]             neighbor_spin_1,
    input wire signed [1:0]             neighbor_spin_2,
    input wire signed [1:0]             neighbor_spin_3,

    input wire        [DATA_WIDTH-1:0]  temperature,
    // REMOVED: input wire [DATA_WIDTH-1:0] rand_num, 
    // We now generate this internally!
    
    output reg signed [1:0]             current_spin_state
);

    localparam FULL_WIDTH = DATA_WIDTH + $clog2(NEIGHBOR_COUNT);

    // --- INTERNAL TRNG INSTANTIATION ---
    wire [15:0] internal_rand_num;
    
    // We instantiate the TRNG we just built.
    // !rst_n because our system uses active-low reset (rst_n), 
    // but our TRNG used active-high (rst).
    trng entropy_source (
        .clk(clk),
        .rst(!rst_n), 
        .rand_out(internal_rand_num)
    );
    // -----------------------------------

    integer i;

    // Internal arrays for cleaner logic
    wire signed [DATA_WIDTH-1:0] j_couplings [0:NEIGHBOR_COUNT-1];
    wire signed [1:0]            neighbor_spins [0:NEIGHBOR_COUNT-1];

    assign j_couplings[0] = j_coupling_0;
    assign j_couplings[1] = j_coupling_1;
    assign j_couplings[2] = j_coupling_2;
    assign j_couplings[3] = j_coupling_3;

    assign neighbor_spins[0] = neighbor_spin_0;
    assign neighbor_spins[1] = neighbor_spin_1;
    assign neighbor_spins[2] = neighbor_spin_2;
    assign neighbor_spins[3] = neighbor_spin_3;

    reg  signed [FULL_WIDTH-1:0]   coupling_energy_sum;
    reg  signed [FULL_WIDTH-1:0]   total_local_energy;
    wire signed [FULL_WIDTH-1:0]   delta_energy;
    wire                           accept_flip;
    wire signed [FULL_WIDTH-1:0]   scaled_temperature;

    always @(*) begin
        coupling_energy_sum = 0;
        for (i = 0; i < NEIGHBOR_COUNT; i = i + 1) begin
            coupling_energy_sum = coupling_energy_sum + (j_couplings[i] * neighbor_spins[i]);
        end
    end

    always @(*) begin
        total_local_energy = coupling_energy_sum + (h_local_field <<< $clog2(NEIGHBOR_COUNT));
    end

    assign delta_energy = (total_local_energy <<< 1) * current_spin_state;
    assign scaled_temperature = temperature <<< $clog2(NEIGHBOR_COUNT);
    
    // Logic: If Delta E < 0, always flip.
    // If Delta E > 0, flip with probability exp(-Delta E / T).
    // We compare internal_rand_num against the threshold.
    assign accept_flip = (delta_energy < 0) || (internal_rand_num < (1'b1 << FRAC_BITS) - (delta_energy / (scaled_temperature + 1)));

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            current_spin_state <= 2'b01; // +1
        end else begin
            if (accept_flip) begin
                current_spin_state <= -current_spin_state;
            end
        end
    end

endmodule
`default_nettype wire