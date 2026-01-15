# ThermoLang Makefile

COMPILER_BUILD_DIR = build
HARDWARE_DIR = hardware
RTL_DIR = $(HARDWARE_DIR)/rtl
SIM_DIR = $(HARDWARE_DIR)/sim
RESULTS_DIR = results

# Tools
IV = iverilog
VVP = vvp
GTKWAVE = gtkwave

# Source Files
VERILOG_SRCS = $(RTL_DIR)/ring_oscillator.v \
               $(RTL_DIR)/trng.v \
               $(RTL_DIR)/spu_core.v \
               $(RTL_DIR)/spu_array_4x4.v \
               $(SIM_DIR)/tb_spu_array.v

# Targets
.PHONY: all clean compiler simulate wave

all: compiler simulate

# 1. Build the ThermoLang C++ Compiler
compiler:
	cmake -B $(COMPILER_BUILD_DIR) -S .
	cmake --build $(COMPILER_BUILD_DIR)

# 2. Run a specific ThermoLang example to generate Hardware Configs
# Usage: make gen-config EXAMPLE=examples/4_domain_specific/3_ising_solver.thermo
gen-config: compiler
	@mkdir -p $(SIM_DIR)
	./$(COMPILER_BUILD_DIR)/thermolangc $(EXAMPLE) --target=fpga
	# Move generated files to simulation directory
	mv *_config.mem $(SIM_DIR)/config.mem
	mv *_schedule.txt $(SIM_DIR)/schedule.txt
	@echo "Hardware configuration generated in $(SIM_DIR)"

# 3. Compile Verilog simulation
sim-build:
	$(IV) -o $(SIM_DIR)/spu_sim.vvp -I $(RTL_DIR) $(VERILOG_SRCS)

# 4. Run Simulation (Generates VCD)
simulate: sim-build
	cd $(SIM_DIR) && $(VVP) spu_sim.vvp
	@echo "Simulation complete. Waveform generated."

# 5. View Waveform
wave:
	$(GTKWAVE) $(SIM_DIR)/spu_trace.vcd $(HARDWARE_DIR)/waves/default_view.gtkw &

clean:
	rm -rf $(COMPILER_BUILD_DIR)
	rm -f $(SIM_DIR)/*.vvp $(SIM_DIR)/*.vcd