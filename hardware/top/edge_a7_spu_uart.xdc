## ThermoLang SPU checkerboard on-board demo constraints

## Clock
set_property -dict { PACKAGE_PIN N11 IOSTANDARD LVCMOS33 } [get_ports { clk }]
create_clock -period 20.000 -name sys_clk [get_ports { clk }]

## Reset button
## Reuse the same working reset pin from your validated TRNG-UART project
set_property -dict { PACKAGE_PIN M14 IOSTANDARD LVCMOS33 PULLDOWN true } [get_ports { rst_btn }]

## LEDs
set_property -dict { PACKAGE_PIN J3 IOSTANDARD LVCMOS33 } [get_ports { led_d3 }]
set_property -dict { PACKAGE_PIN H3 IOSTANDARD LVCMOS33 } [get_ports { led_d4 }]
set_property -dict { PACKAGE_PIN J1 IOSTANDARD LVCMOS33 } [get_ports { led_d5 }]
set_property -dict { PACKAGE_PIN K1 IOSTANDARD LVCMOS33 } [get_ports { led_d6 }]
set_property -dict { PACKAGE_PIN L3 IOSTANDARD LVCMOS33 } [get_ports { led_d8 }]
set_property -dict { PACKAGE_PIN L2 IOSTANDARD LVCMOS33 } [get_ports { led_d9 }]
set_property -dict { PACKAGE_PIN K3 IOSTANDARD LVCMOS33 } [get_ports { led_d10 }]
set_property -dict { PACKAGE_PIN K2 IOSTANDARD LVCMOS33 } [get_ports { led_d11 }]

## UART TX to onboard FT2232H bridge
set_property -dict { PACKAGE_PIN C4 IOSTANDARD LVCMOS33 } [get_ports { uart_tx }]

## Allow intentional ring-oscillator loops in all SPU cores
## These wildcard constraints are the right approach because each spu_core has its own entropy_source
## Disable timing through intentional ring-oscillator loop elements
