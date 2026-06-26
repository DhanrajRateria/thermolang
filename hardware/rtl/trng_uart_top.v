`default_nettype none

module trng_uart_top(
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

    wire [15:0] rand_out;

    reg [31:0] heartbeat_ctr;
    reg [23:0] sample_div;
    reg [15:0] sampled_rand;
    reg        sample_strobe;

    reg [1:0]  tx_state;
    reg        tx_start;
    reg [7:0]  tx_data;
    wire       tx_busy;

    trng u_trng (
        .clk(clk),
        .rst(rst_btn),
        .rand_out(rand_out)
    );

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

    // Sample TRNG slowly enough for visible LED updates and safe UART throughput
    always @(posedge clk or posedge rst_btn) begin
        if (rst_btn) begin
            heartbeat_ctr <= 32'd0;
            sample_div    <= 24'd0;
            sampled_rand  <= 16'd0;
            sample_strobe <= 1'b0;
        end else begin
            heartbeat_ctr <= heartbeat_ctr + 1'b1;
            sample_div    <= sample_div + 1'b1;
            sample_strobe <= 1'b0;

            if (sample_div == 24'd0) begin
                sampled_rand  <= rand_out;
                sample_strobe <= 1'b1;
            end
        end
    end

    // UART packet state machine: A5, low byte, high byte
    always @(posedge clk or posedge rst_btn) begin
        if (rst_btn) begin
            tx_state <= 2'd0;
            tx_start <= 1'b0;
            tx_data  <= 8'h00;
        end else begin
            tx_start <= 1'b0;

            case (tx_state)
                2'd0: begin
                    if (sample_strobe && !tx_busy) begin
                        tx_data  <= 8'hA5;
                        tx_start <= 1'b1;
                        tx_state <= 2'd1;
                    end
                end

                2'd1: begin
                    if (!tx_busy) begin
                        tx_data  <= sampled_rand[7:0];
                        tx_start <= 1'b1;
                        tx_state <= 2'd2;
                    end
                end

                2'd2: begin
                    if (!tx_busy) begin
                        tx_data  <= sampled_rand[15:8];
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
        end
    end

    // Visible indicators
    assign led_d3  = heartbeat_ctr[25];   // heartbeat
    assign led_d4  = sampled_rand[0];
    assign led_d5  = sampled_rand[1];
    assign led_d6  = sampled_rand[2];
    assign led_d8  = sampled_rand[3];
    assign led_d9  = sampled_rand[4];
    assign led_d10 = sampled_rand[5];
    assign led_d11 = sampled_rand[6];

endmodule

`default_nettype wire