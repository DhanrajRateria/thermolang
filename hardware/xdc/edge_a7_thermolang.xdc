## Clock
set_property PACKAGE_PIN N11 [get_ports clk_50mhz]
set_property IOSTANDARD LVCMOS33 [get_ports clk_50mhz]
create_clock -period 20.000 [get_ports clk_50mhz]

## Reset button (SW19)
set_property PACKAGE_PIN M12 [get_ports rst_btn]
set_property IOSTANDARD LVCMOS33 [get_ports rst_btn]

## LEDs
set_property PACKAGE_PIN J3 [get_ports led_d3]
set_property IOSTANDARD LVCMOS33 [get_ports led_d3]

set_property PACKAGE_PIN H3 [get_ports led_d4]
set_property IOSTANDARD LVCMOS33 [get_ports led_d4]

set_property PACKAGE_PIN J1 [get_ports led_d5]
set_property IOSTANDARD LVCMOS33 [get_ports led_d5]

set_property PACKAGE_PIN K1 [get_ports led_d6]
set_property IOSTANDARD LVCMOS33 [get_ports led_d6]

set_property PACKAGE_PIN L3 [get_ports led_d8]
set_property IOSTANDARD LVCMOS33 [get_ports led_d8]

set_property PACKAGE_PIN L2 [get_ports led_d9]
set_property IOSTANDARD LVCMOS33 [get_ports led_d9]

set_property PACKAGE_PIN K3 [get_ports led_d10]
set_property IOSTANDARD LVCMOS33 [get_ports led_d10]

set_property PACKAGE_PIN K2 [get_ports led_d11]
set_property IOSTANDARD LVCMOS33 [get_ports led_d11]

## UART reserved for next phase
#set_property PACKAGE_PIN C4 [get_ports uart_tx]
#set_property IOSTANDARD LVCMOS33 [get_ports uart_tx]
#set_property PACKAGE_PIN D4 [get_ports uart_rx]
#set_property IOSTANDARD LVCMOS33 [get_ports uart_rx]

## Ring oscillator support
set_property ALLOW_COMBINATORIAL_LOOPS TRUE [current_design]