# ThermoLang Makefile

COMPILER_BUILD_DIR = build
HARDWARE_DIR = hardware
RTL_DIR = $(HARDWARE_DIR)/rtl
SIM_DIR = $(HARDWARE_DIR)/sim
TOP_DIR = $(HARDWARE_DIR)/top
XDC_DIR = $(HARDWARE_DIR)/xdc
WAVES_DIR = $(HARDWARE_DIR)/waves

OUT_DIR = out
FPGA_OUT_DIR = $(OUT_DIR)/fpga
RESULTS_DIR = results
ARCHIVE_DIR = archive/generated_root

# Tools
IV = iverilog
VVP = vvp
GTKWAVE = gtkwave

# Default example
EXAMPLE ?= examples/checkerboard.thermo

# Derived names
EXAMPLE_BASE = $(basename $(notdir $(EXAMPLE)))
EXAMPLE_OUT_DIR = $(FPGA_OUT_DIR)/$(EXAMPLE_BASE)

# Source Files
ARRAY_VERILOG_SRCS = $(RTL_DIR)/ring_oscillator.v \
                     $(RTL_DIR)/trng.v \
                     $(RTL_DIR)/spu_core.v \
                     $(RTL_DIR)/spu_array_4x4.v \
                     $(SIM_DIR)/tb_spu_array.v

TRNG_VERILOG_SRCS = $(RTL_DIR)/ring_oscillator.v \
                    $(RTL_DIR)/trng.v \
                    $(SIM_DIR)/tb_trng.v

.PHONY: all compiler test gen-config twin sim-build simulate \
        trng-build trng-sim trng-wave wave clean clean-generated \
        archive-root-generated freeze-check

all: compiler test twin

compiler:
	cmake -B $(COMPILER_BUILD_DIR) -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
	cmake --build $(COMPILER_BUILD_DIR) --parallel

test: compiler
	cd $(COMPILER_BUILD_DIR) && ctest --verbose

gen-config: compiler
	@test -n "$(EXAMPLE)" || (echo "EXAMPLE is required"; exit 1)
	@mkdir -p $(EXAMPLE_OUT_DIR)
	./$(COMPILER_BUILD_DIR)/thermolangc $(EXAMPLE) --target=fpga
	@test -f $(EXAMPLE_BASE)_config.mem || (echo "Missing $(EXAMPLE_BASE)_config.mem"; exit 1)
	@test -f $(EXAMPLE_BASE)_schedule.txt || (echo "Missing $(EXAMPLE_BASE)_schedule.txt"; exit 1)
	mv $(EXAMPLE_BASE)_config.mem $(EXAMPLE_OUT_DIR)/config.mem
	mv $(EXAMPLE_BASE)_schedule.txt $(EXAMPLE_OUT_DIR)/schedule.txt
	@echo "FPGA config written to $(EXAMPLE_OUT_DIR)"

sim-build:
	$(IV) -g2012 -DSIMULATION -I $(RTL_DIR) \
		-o $(SIM_DIR)/sim_twin.vvp \
		$(ARRAY_VERILOG_SRCS)

simulate: sim-build
	cd $(SIM_DIR) && $(VVP) sim_twin.vvp
	@echo "Array simulation complete."

twin: gen-config sim-build
	cp $(EXAMPLE_OUT_DIR)/config.mem $(SIM_DIR)/config.mem
	cp $(EXAMPLE_OUT_DIR)/schedule.txt $(SIM_DIR)/schedule.txt
	cd $(SIM_DIR) && $(VVP) sim_twin.vvp
	@echo "Digital twin complete for $(EXAMPLE)"

trng-build:
	$(IV) -g2012 -DSIMULATION -I $(RTL_DIR) \
		-o $(SIM_DIR)/tb_trng.vvp \
		$(TRNG_VERILOG_SRCS)

trng-sim: trng-build
	cd $(SIM_DIR) && $(VVP) tb_trng.vvp
	@echo "TRNG simulation complete."

trng-wave:
	$(GTKWAVE) $(SIM_DIR)/trng_waves.vcd &

wave:
	$(GTKWAVE) $(SIM_DIR)/spu_trace.vcd $(WAVES_DIR)/default_view.gtkw &

archive-root-generated:
	@mkdir -p $(ARCHIVE_DIR)
	@sh -c 'for f in *_config.mem *_schedule.txt *_sim.py *_thrml.py *.ir; do \
		[ -e "$$f" ] && mv "$$f" $(ARCHIVE_DIR)/; \
	done'
	@echo "Archived root-generated artifacts to $(ARCHIVE_DIR)"

freeze-check:
	@test -d $(COMPILER_BUILD_DIR) || (echo "Build dir missing"; exit 1)
	@test -f $(SIM_DIR)/tb_trng.v || (echo "Missing $(SIM_DIR)/tb_trng.v"; exit 1)
	@test -f docs/fpga_contract.md || (echo "Missing docs/fpga_contract.md"; exit 1)
	@test -f $(TOP_DIR)/trng_top.v || (echo "Missing $(TOP_DIR)/trng_top.v"; exit 1)
	@test -f $(XDC_DIR)/edge_a7_thermolang.xdc || (echo "Missing $(XDC_DIR)/edge_a7_thermolang.xdc"; exit 1)
	@echo "Freeze check passed."

clean-generated:
	rm -rf $(OUT_DIR)
	rm -f $(SIM_DIR)/config.mem $(SIM_DIR)/schedule.txt
	rm -f $(SIM_DIR)/*.vvp $(SIM_DIR)/*.vcd

clean:
	rm -rf $(COMPILER_BUILD_DIR)
	rm -rf $(OUT_DIR)
	rm -f $(SIM_DIR)/config.mem $(SIM_DIR)/schedule.txt
	rm -f $(SIM_DIR)/*.vvp $(SIM_DIR)/*.vcd