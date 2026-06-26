`default_nettype none

module spu_fpga_top(
    input  wire clk,
    input  wire rst_btn,

    output wire led_d3,
    output wire led_d4,
    output wire led_d5,
    output wire led_d6,
    output wire led_d8,
    output wire led_d9,
    output wire led_d10,
    output wire led_d11,

    output wire uart_tx
);

    localparam integer DATA_WIDTH   = 16;
    localparam integer FRAC_BITS    = 12;
    localparam integer CONFIG_WORDS = 80;          // 16 cells * 5 words/cell
    localparam integer PHASE_DIV    = 50000;       // 1 ms half-phase at 50 MHz
    localparam integer MAX_SWEEPS   = 2000;        // ~4 seconds total
    localparam [15:0] TEMP_INIT     = 16'd28391;   // 6.93147 * 4096
    localparam [15:0] COOL_MULT     = 16'd4055;    // 0.99 * 4096
    localparam integer UART_PERIOD  = 25000000;    // ~0.5 s at 50 MHz

    localparam [1:0]
        ST_RESET   = 2'd0,
        ST_PROGRAM = 2'd1,
        ST_RUN     = 2'd2,
        ST_DONE    = 2'd3;

    reg [1:0] state;

    reg rst_n_int;
    reg phase;

    reg [31:0] heartbeat_ctr;
    reg [31:0] phase_div_ctr;
    reg [31:0] uart_period_ctr;

    reg [15:0] temperature;
    reg [15:0] sweep_count;
    reg [7:0]  reset_hold_ctr;
    reg [4:0]  prog_cell;

    reg signed [DATA_WIDTH-1:0] config_rom [0:CONFIG_WORDS-1];

    initial begin
        $readmemh("config.mem", config_rom);
    end

    wire [6:0] mem_base = prog_cell * 7'd5;

    wire [3:0] config_addr_row = {2'b00, prog_cell[4:2]};
    wire [3:0] config_addr_col = {2'b00, prog_cell[1:0]};
    wire config_we = (state == ST_PROGRAM);

    wire signed [DATA_WIDTH-1:0] config_data_in_h       = config_rom[mem_base + 0];
    wire signed [DATA_WIDTH-1:0] config_data_in_j_north = config_rom[mem_base + 1];
    wire signed [DATA_WIDTH-1:0] config_data_in_j_south = config_rom[mem_base + 2];
    wire signed [DATA_WIDTH-1:0] config_data_in_j_east  = config_rom[mem_base + 3];
    wire signed [DATA_WIDTH-1:0] config_data_in_j_west  = config_rom[mem_base + 4];

    wire signed [1:0] spin_out_0_0, spin_out_0_1, spin_out_0_2, spin_out_0_3;
    wire signed [1:0] spin_out_1_0, spin_out_1_1, spin_out_1_2, spin_out_1_3;
    wire signed [1:0] spin_out_2_0, spin_out_2_1, spin_out_2_2, spin_out_2_3;
    wire signed [1:0] spin_out_3_0, spin_out_3_1, spin_out_3_2, spin_out_3_3;

    // UART
    reg  [1:0] tx_state;
    reg        tx_start;
    reg  [7:0] tx_data;
    wire       tx_busy;

    uart_tx #(
        .CLK_HZ(50000000),
        .BAUD(9600)
    ) u_uart_tx (
        .clk(clk),
        .rst(rst_btn),
        .data_in(tx_data),
        .start(tx_start),
        .tx(uart_tx),
        .busy(tx_busy)
    );

    spu_array_4x4 #(
        .DATA_WIDTH(DATA_WIDTH),
        .FRAC_BITS(FRAC_BITS)
    ) u_array (
        .clk(clk),
        .rst_n(rst_n_int),
        .phase(phase),

        .config_addr_row(config_addr_row),
        .config_addr_col(config_addr_col),
        .config_data_in_h(config_data_in_h),
        .config_data_in_j_north(config_data_in_j_north),
        .config_data_in_j_south(config_data_in_j_south),
        .config_data_in_j_east(config_data_in_j_east),
        .config_data_in_j_west(config_data_in_j_west),
        .config_we(config_we),

        .temperature(temperature),

        .spin_out_0_0(spin_out_0_0), .spin_out_0_1(spin_out_0_1), .spin_out_0_2(spin_out_0_2), .spin_out_0_3(spin_out_0_3),
        .spin_out_1_0(spin_out_1_0), .spin_out_1_1(spin_out_1_1), .spin_out_1_2(spin_out_1_2), .spin_out_1_3(spin_out_1_3),
        .spin_out_2_0(spin_out_2_0), .spin_out_2_1(spin_out_2_1), .spin_out_2_2(spin_out_2_2), .spin_out_2_3(spin_out_2_3),
        .spin_out_3_0(spin_out_3_0), .spin_out_3_1(spin_out_3_1), .spin_out_3_2(spin_out_3_2), .spin_out_3_3(spin_out_3_3)
    );

    // Pack final 16-spin bitmap: bit = 1 if spin is +1
    wire [15:0] final_state_bitmap;
    assign final_state_bitmap[0]  = (spin_out_0_0 == 2'sd1);
    assign final_state_bitmap[1]  = (spin_out_0_1 == 2'sd1);
    assign final_state_bitmap[2]  = (spin_out_0_2 == 2'sd1);
    assign final_state_bitmap[3]  = (spin_out_0_3 == 2'sd1);
    assign final_state_bitmap[4]  = (spin_out_1_0 == 2'sd1);
    assign final_state_bitmap[5]  = (spin_out_1_1 == 2'sd1);
    assign final_state_bitmap[6]  = (spin_out_1_2 == 2'sd1);
    assign final_state_bitmap[7]  = (spin_out_1_3 == 2'sd1);
    assign final_state_bitmap[8]  = (spin_out_2_0 == 2'sd1);
    assign final_state_bitmap[9]  = (spin_out_2_1 == 2'sd1);
    assign final_state_bitmap[10] = (spin_out_2_2 == 2'sd1);
    assign final_state_bitmap[11] = (spin_out_2_3 == 2'sd1);
    assign final_state_bitmap[12] = (spin_out_3_0 == 2'sd1);
    assign final_state_bitmap[13] = (spin_out_3_1 == 2'sd1);
    assign final_state_bitmap[14] = (spin_out_3_2 == 2'sd1);
    assign final_state_bitmap[15] = (spin_out_3_3 == 2'sd1);

    // Main control FSM
    always @(posedge clk or posedge rst_btn) begin
        if (rst_btn) begin
            state          <= ST_RESET;
            rst_n_int      <= 1'b0;
            phase          <= 1'b0;
            heartbeat_ctr  <= 32'd0;
            phase_div_ctr  <= 32'd0;
            uart_period_ctr<= 32'd0;
            temperature    <= TEMP_INIT;
            sweep_count    <= 16'd0;
            reset_hold_ctr <= 8'd0;
            prog_cell      <= 5'd0;
        end else begin
            heartbeat_ctr <= heartbeat_ctr + 1'b1;

            case (state)
                ST_RESET: begin
                    rst_n_int <= 1'b0;
                    phase <= 1'b0;
                    temperature <= TEMP_INIT;
                    sweep_count <= 16'd0;
                    phase_div_ctr <= 32'd0;
                    prog_cell <= 5'd0;

                    if (reset_hold_ctr == 8'd50) begin
                        reset_hold_ctr <= 8'd0;
                        rst_n_int <= 1'b1;
                        state <= ST_PROGRAM;
                    end else begin
                        reset_hold_ctr <= reset_hold_ctr + 1'b1;
                    end
                end

                ST_PROGRAM: begin
                    rst_n_int <= 1'b1;
                    if (prog_cell == 5'd15) begin
                        prog_cell <= 5'd0;
                        phase_div_ctr <= 32'd0;
                        state <= ST_RUN;
                    end else begin
                        prog_cell <= prog_cell + 1'b1;
                    end
                end

                ST_RUN: begin
                    rst_n_int <= 1'b1;
                    if (phase_div_ctr == PHASE_DIV - 1) begin
                        phase_div_ctr <= 32'd0;
                        phase <= ~phase;

                        // Update temperature after every full phase pair
                        if (phase == 1'b1) begin
                            sweep_count <= sweep_count + 1'b1;
                            temperature <= (temperature * COOL_MULT) >> FRAC_BITS;

                            if (sweep_count == MAX_SWEEPS - 1) begin
                                state <= ST_DONE;
                                phase <= 1'b0;
                                uart_period_ctr <= 32'd0;
                            end
                        end
                    end else begin
                        phase_div_ctr <= phase_div_ctr + 1'b1;
                    end
                end

                ST_DONE: begin
                    rst_n_int <= 1'b1;
                    phase <= 1'b0;
                
                    if (uart_period_ctr == UART_PERIOD - 1)
                        uart_period_ctr <= 32'd0;
                    else
                        uart_period_ctr <= uart_period_ctr + 1'b1;
                end

                default: state <= ST_RESET;
            endcase
        end
    end

    // UART packet after DONE: 0x5A, bitmap[7:0], bitmap[15:8]
    always @(posedge clk or posedge rst_btn) begin
        if (rst_btn) begin
            tx_state <= 2'd0;
            tx_start <= 1'b0;
            tx_data  <= 8'h00;
        end else begin
            tx_start <= 1'b0;

            if (state == ST_DONE) begin
                case (tx_state)
                    2'd0: begin
                        if (!tx_busy && uart_period_ctr == 32'd0) begin
                            tx_data  <= 8'h5A;
                            tx_start <= 1'b1;
                            tx_state <= 2'd1;
                        end
                    end

                    2'd1: begin
                        if (!tx_busy) begin
                            tx_data  <= final_state_bitmap[7:0];
                            tx_start <= 1'b1;
                            tx_state <= 2'd2;
                        end
                    end

                    2'd2: begin
                        if (!tx_busy) begin
                            tx_data  <= final_state_bitmap[15:8];
                            tx_start <= 1'b1;
                            tx_state <= 2'd3;
                        end
                    end

                    2'd3: begin
                        if (!tx_busy) begin
                            tx_state <= 2'd0;
                        end
                    end

                    default: tx_state <= 2'd0;
                endcase
            end else begin
                tx_state <= 2'd0;
            end
        end
    end

    // LEDs: 4 status + 4 representative spins
    assign led_d3  = heartbeat_ctr[25];     // heartbeat
    assign led_d4  = (state == ST_DONE);    // done
    assign led_d5  = phase;                 // phase indicator
    assign led_d6  = (state == ST_RUN);     // run active

    assign led_d8  = (spin_out_0_0 == 2'sd1);
    assign led_d9  = (spin_out_0_1 == 2'sd1);
    assign led_d10 = (spin_out_1_0 == 2'sd1);
    assign led_d11 = (spin_out_1_1 == 2'sd1);

endmodule

`default_nettype wire