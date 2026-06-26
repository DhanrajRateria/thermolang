# Intentional TRNG ring-oscillator loops
# Apply only during implementation

set_property ALLOW_COMBINATORIAL_LOOPS TRUE [get_nets -hier -filter {NAME =~ "*entropy_source/ro1_inst/w1"}]
set_property ALLOW_COMBINATORIAL_LOOPS TRUE [get_nets -hier -filter {NAME =~ "*entropy_source/ro2_inst/w1"}]
set_property ALLOW_COMBINATORIAL_LOOPS TRUE [get_nets -hier -filter {NAME =~ "*entropy_source/ro3_inst/w1"}]
set_property ALLOW_COMBINATORIAL_LOOPS TRUE [get_nets -hier -filter {NAME =~ "*entropy_source/ro4_inst/w1"}]