## ThermoLang TRNG bring-up constraints for slowed TRNG display top

## Clock
set_property -dict { PACKAGE_PIN N11 IOSTANDARD LVCMOS33 } [get_ports { clk }]
create_clock -period 20.000 -name sys_clk [get_ports { clk }]

## Reset button
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

# Allow intentional ring-oscillator combinational loops
set_property ALLOW_COMBINATORIAL_LOOPS TRUE [get_nets {u_trng/ro1_inst/w1}]
set_property ALLOW_COMBINATORIAL_LOOPS TRUE [get_nets {u_trng/ro2_inst/w1}]
set_property ALLOW_COMBINATORIAL_LOOPS TRUE [get_nets {u_trng/ro3_inst/w1}]
set_property ALLOW_COMBINATORIAL_LOOPS TRUE [get_nets {u_trng/ro4_inst/w1}]