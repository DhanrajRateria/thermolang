`default_nettype none

// This file is correct. No changes needed from the previous version.

module spu_array_4x4 #(
    parameter DATA_WIDTH = 16,
    parameter FRAC_BITS  = 12
)(
    input wire                      clk,
    input wire                      rst_n,
    input wire [3:0]                config_addr_row,
    input wire [3:0]                config_addr_col,
    input wire signed [DATA_WIDTH-1:0]  config_data_in_h,
    input wire signed [DATA_WIDTH-1:0]  config_data_in_j_north,
    input wire signed [DATA_WIDTH-1:0]  config_data_in_j_south,
    input wire signed [DATA_WIDTH-1:0]  config_data_in_j_east,
    input wire signed [DATA_WIDTH-1:0]  config_data_in_j_west,
    input wire                      config_we,
    input wire        [DATA_WIDTH-1:0]  temperature,

    // Flattened 16x random number inputs
    input wire [DATA_WIDTH-1:0] rand_num_0_0, rand_num_0_1, rand_num_0_2, rand_num_0_3,
    input wire [DATA_WIDTH-1:0] rand_num_1_0, rand_num_1_1, rand_num_1_2, rand_num_1_3,
    input wire [DATA_WIDTH-1:0] rand_num_2_0, rand_num_2_1, rand_num_2_2, rand_num_2_3,
    input wire [DATA_WIDTH-1:0] rand_num_3_0, rand_num_3_1, rand_num_3_2, rand_num_3_3,

    // Flattened 16x spin outputs
    output wire signed [1:0] spin_out_0_0, spin_out_0_1, spin_out_0_2, spin_out_0_3,
    output wire signed [1:0] spin_out_1_0, spin_out_1_1, spin_out_1_2, spin_out_1_3,
    output wire signed [1:0] spin_out_2_0, spin_out_2_1, spin_out_2_2, spin_out_2_3,
    output wire signed [1:0] spin_out_3_0, spin_out_3_1, spin_out_3_2, spin_out_3_3
);

    reg signed [DATA_WIDTH-1:0] h_matrix [0:3][0:3];
    reg signed [DATA_WIDTH-1:0] j_matrix_n [0:3][0:3];
    reg signed [DATA_WIDTH-1:0] j_matrix_s [0:3][0:3];
    reg signed [DATA_WIDTH-1:0] j_matrix_e [0:3][0:3];
    reg signed [DATA_WIDTH-1:0] j_matrix_w [0:3][0:3];

    wire [DATA_WIDTH-1:0] rand_num_grid [0:3][0:3];
    wire signed [1:0]     spin_out_grid [0:3][0:3];

    // Mapping logic for flattened ports to internal arrays
    assign rand_num_grid[0][0] = rand_num_0_0; assign rand_num_grid[0][1] = rand_num_0_1;
    assign rand_num_grid[0][2] = rand_num_0_2; assign rand_num_grid[0][3] = rand_num_0_3;
    assign rand_num_grid[1][0] = rand_num_1_0; assign rand_num_grid[1][1] = rand_num_1_1;
    assign rand_num_grid[1][2] = rand_num_1_2; assign rand_num_grid[1][3] = rand_num_1_3;
    assign rand_num_grid[2][0] = rand_num_2_0; assign rand_num_grid[2][1] = rand_num_2_1;
    assign rand_num_grid[2][2] = rand_num_2_2; assign rand_num_grid[2][3] = rand_num_2_3;
    assign rand_num_grid[3][0] = rand_num_3_0; assign rand_num_grid[3][1] = rand_num_3_1;
    assign rand_num_grid[3][2] = rand_num_3_2; assign rand_num_grid[3][3] = rand_num_3_3;

    assign spin_out_0_0 = spin_out_grid[0][0]; assign spin_out_0_1 = spin_out_grid[0][1];
    assign spin_out_0_2 = spin_out_grid[0][2]; assign spin_out_0_3 = spin_out_grid[0][3];
    assign spin_out_1_0 = spin_out_grid[1][0]; assign spin_out_1_1 = spin_out_grid[1][1];
    assign spin_out_1_2 = spin_out_grid[1][2]; assign spin_out_1_3 = spin_out_grid[1][3];
    assign spin_out_2_0 = spin_out_grid[2][0]; assign spin_out_2_1 = spin_out_grid[2][1];
    assign spin_out_2_2 = spin_out_grid[2][2]; assign spin_out_2_3 = spin_out_grid[2][3];
    assign spin_out_3_0 = spin_out_grid[3][0]; assign spin_out_3_1 = spin_out_grid[3][1];
    assign spin_out_3_2 = spin_out_grid[3][2]; assign spin_out_3_3 = spin_out_grid[3][3];

    always @(posedge clk) begin
        if (config_we) begin
            h_matrix[config_addr_row][config_addr_col]   <= config_data_in_h;
            j_matrix_n[config_addr_row][config_addr_col] <= config_data_in_j_north;
            j_matrix_s[config_addr_row][config_addr_col] <= config_data_in_j_south;
            j_matrix_e[config_addr_row][config_addr_col] <= config_data_in_j_east;
            j_matrix_w[config_addr_row][config_addr_col] <= config_data_in_j_west;
        end
    end

    genvar r, c;
    generate
        for (r = 0; r < 4; r = r + 1) begin
            for (c = 0; c < 4; c = c + 1) begin
                spu_core #(
                    .DATA_WIDTH(DATA_WIDTH), .FRAC_BITS(FRAC_BITS)
                ) spu_core_inst (
                    .clk(clk), .rst_n(rst_n),
                    .h_local_field(h_matrix[r][c]),
                    .j_coupling_0(j_matrix_n[r][c]),
                    .j_coupling_1(j_matrix_s[r][c]),
                    .j_coupling_2(j_matrix_e[r][c]),
                    .j_coupling_3(j_matrix_w[r][c]),
                    .neighbor_spin_0(spin_out_grid[(r == 0) ? 3 : r-1][c]),
                    .neighbor_spin_1(spin_out_grid[(r == 3) ? 0 : r+1][c]),
                    .neighbor_spin_2(spin_out_grid[r][(c == 3) ? 0 : c+1]),
                    .neighbor_spin_3(spin_out_grid[r][(c == 0) ? 3 : c-1]),
                    .temperature(temperature),
                    .rand_num(rand_num_grid[r][c]),
                    .current_spin_state(spin_out_grid[r][c])
                );
            end
        end
    endgenerate

endmodule
`default_nettype wire