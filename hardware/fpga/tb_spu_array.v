`default_nettype none
`timescale 1ns / 1ps

`include "spu_core.v"
`include "spu_array_4x4.v"

module tb_spu_array;

    parameter DATA_WIDTH = 16;
    parameter FRAC_BITS  = 12;
    parameter CLK_PERIOD = 10;
    parameter GRID_SIZE = 4;
    parameter CONFIG_WORDS_PER_SPIN = 5;
    parameter TOTAL_CONFIG_WORDS = GRID_SIZE * GRID_SIZE * CONFIG_WORDS_PER_SPIN;

    reg clk;
    reg rst_n;
    reg [3:0]  config_addr_row;
    reg [3:0]  config_addr_col;
    reg signed [DATA_WIDTH-1:0] config_data_in_h, config_data_in_j_north, config_data_in_j_south, config_data_in_j_east, config_data_in_j_west;
    reg        config_we;
    reg        [DATA_WIDTH-1:0] temperature;
    
    reg [DATA_WIDTH-1:0] rand_num_0_0, rand_num_0_1, rand_num_0_2, rand_num_0_3, rand_num_1_0, rand_num_1_1, rand_num_1_2, rand_num_1_3, rand_num_2_0, rand_num_2_1, rand_num_2_2, rand_num_2_3, rand_num_3_0, rand_num_3_1, rand_num_3_2, rand_num_3_3;
    wire signed [1:0] spin_out_0_0, spin_out_0_1, spin_out_0_2, spin_out_0_3, spin_out_1_0, spin_out_1_1, spin_out_1_2, spin_out_1_3, spin_out_2_0, spin_out_2_1, spin_out_2_2, spin_out_2_3, spin_out_3_0, spin_out_3_1, spin_out_3_2, spin_out_3_3;
    
    wire signed [1:0] spin_out_grid [0:3][0:3];
    assign spin_out_grid[0][0] = spin_out_0_0; assign spin_out_grid[0][1] = spin_out_0_1; assign spin_out_grid[0][2] = spin_out_0_2; assign spin_out_grid[0][3] = spin_out_0_3;
    assign spin_out_grid[1][0] = spin_out_1_0; assign spin_out_grid[1][1] = spin_out_1_1; assign spin_out_grid[1][2] = spin_out_1_2; assign spin_out_grid[1][3] = spin_out_1_3;
    assign spin_out_grid[2][0] = spin_out_2_0; assign spin_out_grid[2][1] = spin_out_2_1; assign spin_out_grid[2][2] = spin_out_2_2; assign spin_out_grid[2][3] = spin_out_2_3;
    assign spin_out_grid[3][0] = spin_out_3_0; assign spin_out_grid[3][1] = spin_out_3_1; assign spin_out_grid[3][2] = spin_out_3_2; assign spin_out_grid[3][3] = spin_out_3_3;

    spu_array_4x4 #(.DATA_WIDTH(DATA_WIDTH), .FRAC_BITS(FRAC_BITS)) dut (
        .clk(clk), .rst_n(rst_n),
        .config_addr_row(config_addr_row), .config_addr_col(config_addr_col),
        .config_data_in_h(config_data_in_h), .config_data_in_j_north(config_data_in_j_north), .config_data_in_j_south(config_data_in_j_south),
        .config_data_in_j_east(config_data_in_j_east), .config_data_in_j_west(config_data_in_j_west), .config_we(config_we),
        .temperature(temperature),
        .rand_num_0_0(rand_num_0_0), .rand_num_0_1(rand_num_0_1), .rand_num_0_2(rand_num_0_2), .rand_num_0_3(rand_num_0_3),
        .rand_num_1_0(rand_num_1_0), .rand_num_1_1(rand_num_1_1), .rand_num_1_2(rand_num_1_2), .rand_num_1_3(rand_num_1_3),
        .rand_num_2_0(rand_num_2_0), .rand_num_2_1(rand_num_2_1), .rand_num_2_2(rand_num_2_2), .rand_num_2_3(rand_num_2_3),
        .rand_num_3_0(rand_num_3_0), .rand_num_3_1(rand_num_3_1), .rand_num_3_2(rand_num_3_2), .rand_num_3_3(rand_num_3_3),
        .spin_out_0_0(spin_out_0_0), .spin_out_0_1(spin_out_0_1), .spin_out_0_2(spin_out_0_2), .spin_out_0_3(spin_out_0_3),
        .spin_out_1_0(spin_out_1_0), .spin_out_1_1(spin_out_1_1), .spin_out_1_2(spin_out_1_2), .spin_out_1_3(spin_out_1_3),
        .spin_out_2_0(spin_out_2_0), .spin_out_2_1(spin_out_2_1), .spin_out_2_2(spin_out_2_2), .spin_out_2_3(spin_out_2_3),
        .spin_out_3_0(spin_out_3_0), .spin_out_3_1(spin_out_3_1), .spin_out_3_2(spin_out_3_2), .spin_out_3_3(spin_out_3_3)
    );

    always #(CLK_PERIOD / 2) clk = ~clk;
    
    real initial_temp_real, cooling_rate_real, temp_real;
    integer anneal_steps, file_handle, mem_idx, r, c, step;
    reg [DATA_WIDTH-1:0] problem_mem [0:TOTAL_CONFIG_WORDS-1];

    integer dummy_fscan_ret;

    function real fixed_to_real(input signed [DATA_WIDTH-1:0] val);
        fixed_to_real = val / (1 << FRAC_BITS);
    endfunction
    
    task display_spin_grid;
        begin
            $display("--- Spin Grid at time %t ---", $time);
            for (r = 0; r < GRID_SIZE; r = r + 1) begin
                $write("  ROW %d: ", r);
                for (c = 0; c < GRID_SIZE; c = c + 1) begin
                    if (spin_out_grid[r][c] == 1) $write("+ "); else $write("- ");
                end
                $write("\n");
            end
            $display("------------------------------");
        end
    endtask
    
    initial begin
        clk=0; rst_n=0; config_we=0; #(CLK_PERIOD*2); rst_n=1;
        
        file_handle = $fopen("3_ising_solver_schedule.txt", "r");
        if (file_handle == 0) begin $display("ERROR: Could not open 3_ising_solver_schedule.txt"); $finish; end
        dummy_fscan_ret = $fscanf(file_handle, "%*s %f", initial_temp_real); 
        dummy_fscan_ret = $fscanf(file_handle, "%*s %f", cooling_rate_real);
        dummy_fscan_ret = $fscanf(file_handle, "%*s %d", anneal_steps);
        $fclose(file_handle);
        $readmemh("3_ising_solver_config.mem", problem_mem);
        
        mem_idx = 0; config_we = 1;
        for(r=0; r<GRID_SIZE; r=r+1) for(c=0; c<GRID_SIZE; c=c+1) begin
            config_addr_row<=r; config_addr_col<=c;
            config_data_in_h<=problem_mem[mem_idx]; mem_idx=mem_idx+1;
            config_data_in_j_north<=problem_mem[mem_idx]; mem_idx=mem_idx+1;
            config_data_in_j_south<=problem_mem[mem_idx]; mem_idx=mem_idx+1;
            config_data_in_j_east<=problem_mem[mem_idx]; mem_idx=mem_idx+1;
            config_data_in_j_west<=problem_mem[mem_idx]; mem_idx=mem_idx+1;
            #CLK_PERIOD;
        end
        config_we = 0;
        
        temperature = initial_temp_real * (1 << FRAC_BITS);
        for (step = 0; step < anneal_steps; step = step + 1) begin
            rand_num_0_0 <= {$random, $random}; rand_num_0_1 <= {$random, $random}; rand_num_0_2 <= {$random, $random}; rand_num_0_3 <= {$random, $random};
            rand_num_1_0 <= {$random, $random}; rand_num_1_1 <= {$random, $random}; rand_num_1_2 <= {$random, $random}; rand_num_1_3 <= {$random, $random};
            rand_num_2_0 <= {$random, $random}; rand_num_2_1 <= {$random, $random}; rand_num_2_2 <= {$random, $random}; rand_num_2_3 <= {$random, $random};
            rand_num_3_0 <= {$random, $random}; rand_num_3_1 <= {$random, $random}; rand_num_3_2 <= {$random, $random}; rand_num_3_3 <= {$random, $random};
            if (step % (anneal_steps/10 + 1) == 0) begin
                $display("[TB] Step %d, Temp = %f", step, fixed_to_real(temperature));
                display_spin_grid();
            end
            
            temp_real = fixed_to_real(temperature);
            temperature <= (temp_real * cooling_rate_real) * (1 << FRAC_BITS);
            #(CLK_PERIOD * 5);
        end
        
        $display("[TB] Annealing finished."); display_spin_grid();
        $write("[FINAL_STATE]: [");
        for(r=0; r<GRID_SIZE; r=r+1) for(c=0; c<GRID_SIZE; c=c+1) begin
            if (spin_out_grid[r][c] == 1) $write("1"); else $write("-1");
            if (r < GRID_SIZE - 1 || c < GRID_SIZE - 1) $write(", ");
        end
        $write("]\n");
        $finish;
    end
endmodule
`default_nettype wire