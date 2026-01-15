`default_nettype none
`timescale 1ns / 1ps

module tb_spu_array;

    parameter DATA_WIDTH = 16;
    parameter FRAC_BITS  = 12;
    parameter CLK_PERIOD = 10;
    parameter GRID_SIZE = 4;
    parameter TOTAL_CONFIG_WORDS = 128; 

    reg clk;
    reg rst_n;
    
    reg [3:0]  config_addr_row;
    reg [3:0]  config_addr_col;
    reg signed [DATA_WIDTH-1:0] config_data_in_h, config_data_in_j_north, config_data_in_j_south, config_data_in_j_east, config_data_in_j_west;
    reg        config_we;
    reg        [DATA_WIDTH-1:0] temperature;
    
    wire signed [1:0] spin_out_grid [0:3][0:3];
    wire signed [1:0] so00, so01, so02, so03, so10, so11, so12, so13, so20, so21, so22, so23, so30, so31, so32, so33;
    
    assign spin_out_grid[0][0] = so00; assign spin_out_grid[0][1] = so01; assign spin_out_grid[0][2] = so02; assign spin_out_grid[0][3] = so03;
    assign spin_out_grid[1][0] = so10; assign spin_out_grid[1][1] = so11; assign spin_out_grid[1][2] = so12; assign spin_out_grid[1][3] = so13;
    assign spin_out_grid[2][0] = so20; assign spin_out_grid[2][1] = so21; assign spin_out_grid[2][2] = so22; assign spin_out_grid[2][3] = so23;
    assign spin_out_grid[3][0] = so30; assign spin_out_grid[3][1] = so31; assign spin_out_grid[3][2] = so32; assign spin_out_grid[3][3] = so33;

    spu_array_4x4 #(.DATA_WIDTH(DATA_WIDTH), .FRAC_BITS(FRAC_BITS)) dut (
        .clk(clk), .rst_n(rst_n),
        .config_addr_row(config_addr_row), .config_addr_col(config_addr_col),
        .config_data_in_h(config_data_in_h), .config_data_in_j_north(config_data_in_j_north), 
        .config_data_in_j_south(config_data_in_j_south), .config_data_in_j_east(config_data_in_j_east), 
        .config_data_in_j_west(config_data_in_j_west), .config_we(config_we),
        .temperature(temperature),
        .spin_out_0_0(so00), .spin_out_0_1(so01), .spin_out_0_2(so02), .spin_out_0_3(so03),
        .spin_out_1_0(so10), .spin_out_1_1(so11), .spin_out_1_2(so12), .spin_out_1_3(so13),
        .spin_out_2_0(so20), .spin_out_2_1(so21), .spin_out_2_2(so22), .spin_out_2_3(so23),
        .spin_out_3_0(so30), .spin_out_3_1(so31), .spin_out_3_2(so32), .spin_out_3_3(so33)
    );

    always #(CLK_PERIOD / 2) clk = ~clk;
    
    real initial_temp_real, cooling_rate_real, temp_real;
    integer anneal_steps, file_handle, mem_idx, r, c, step;
    reg [DATA_WIDTH-1:0] problem_mem [0:TOTAL_CONFIG_WORDS-1];
    integer dummy_fscan_ret;

    function real fixed_to_real(input signed [DATA_WIDTH-1:0] val);
        fixed_to_real = val / (1.0 * (1 << FRAC_BITS));
    endfunction
    
    initial begin
        clk=0; rst_n=0; config_we=0; 
        #(CLK_PERIOD*20); 
        rst_n=1;
        
        file_handle = $fopen("schedule.txt", "r");
        if (file_handle == 0) begin
             $display("ERROR: Could not open schedule.txt");
             $finish;
        end
        dummy_fscan_ret = $fscanf(file_handle, "%*s %f", initial_temp_real); 
        dummy_fscan_ret = $fscanf(file_handle, "%*s %f", cooling_rate_real);
        dummy_fscan_ret = $fscanf(file_handle, "%*s %d", anneal_steps);
        $fclose(file_handle);
        
        // Initialize memory with zeros to avoid X propagation
        for (mem_idx=0; mem_idx<TOTAL_CONFIG_WORDS; mem_idx=mem_idx+1) problem_mem[mem_idx] = 0;

        $readmemh("config.mem", problem_mem);
        
        $display("[TB] Programming Array...");
        mem_idx = 0; config_we = 1;
        for(r=0; r<GRID_SIZE; r=r+1) begin
            for(c=0; c<GRID_SIZE; c=c+1) begin
                config_addr_row<=r; config_addr_col<=c;
                config_data_in_h<=problem_mem[mem_idx]; mem_idx=mem_idx+1;
                config_data_in_j_north<=problem_mem[mem_idx]; mem_idx=mem_idx+1;
                config_data_in_j_south<=problem_mem[mem_idx]; mem_idx=mem_idx+1;
                config_data_in_j_east<=problem_mem[mem_idx]; mem_idx=mem_idx+1;
                config_data_in_j_west<=problem_mem[mem_idx]; mem_idx=mem_idx+1;
                #(CLK_PERIOD);
            end
        end
        config_we = 0;
        
        temperature = initial_temp_real * (1 << FRAC_BITS);
        $display("[TB] Starting Anneal. T_start=%f, Steps=%d", initial_temp_real, anneal_steps);
        
        // Limit steps for simulation speed if needed, but run at least some
        if (anneal_steps > 10000) anneal_steps = 2000;

        for (step = 0; step < anneal_steps; step = step + 1) begin
            temp_real = fixed_to_real(temperature);
            temperature <= (temp_real * cooling_rate_real) * (1 << FRAC_BITS);
            #(CLK_PERIOD * 10);
        end
        
        $display("[TB] Annealing finished.");
        $write("[FINAL_STATE]: [");
        for(r=0; r<GRID_SIZE; r=r+1) for(c=0; c<GRID_SIZE; c=c+1) begin
            if (spin_out_grid[r][c] == 1) $write("1"); else $write("-1");
            if (r < GRID_SIZE - 1 || c < GRID_SIZE - 1) $write(", ");
        end
        $write("]\n");
        $finish;
    end
endmodule