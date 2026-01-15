import subprocess
import shutil
import os
import sys

RTL_DIR = "hardware/rtl"
SIM_DIR = "hardware/sim"
BUILD_DIR = "build"

def run_twin(thermo_file):
    print(f"--- Running Digital Twin for {thermo_file} ---")
    
    if not os.path.exists(SIM_DIR):
        os.makedirs(SIM_DIR)

    # 1. Compile source to FPGA config
    base_name = os.path.splitext(os.path.basename(thermo_file))[0]
    compiler = f"./{BUILD_DIR}/thermolangc"
    
    print(f"[1/4] Compiling {thermo_file}...")
    try:
        subprocess.run([compiler, thermo_file, "--target=fpga"], check=True)
    except subprocess.CalledProcessError:
        print(">>> FAIL: Compilation failed.")
        return

    # 2. Move config files
    try:
        shutil.move(f"{base_name}_config.mem", f"{SIM_DIR}/config.mem")
        shutil.move(f"{base_name}_schedule.txt", f"{SIM_DIR}/schedule.txt")
        print(f"[2/4] Config files moved to {SIM_DIR}")
    except FileNotFoundError:
        print(f">>> FAIL: Compiler did not produce expected {base_name}_config.mem")
        return

    # 3. Compile Verilog
    print("[3/4] Compiling Verilog Testbench...")
    iverilog_cmd = [
        "iverilog", 
        "-o", f"{SIM_DIR}/sim_twin",
        "-I", RTL_DIR,
        f"{SIM_DIR}/tb_spu_array.v",
        f"{RTL_DIR}/spu_array_4x4.v",
        f"{RTL_DIR}/spu_core.v",
        f"{RTL_DIR}/trng.v",
        f"{RTL_DIR}/ring_oscillator.v"
    ]
    
    res_cmp = subprocess.run(iverilog_cmd, capture_output=True, text=True)
    if res_cmp.returncode != 0:
        print(">>> FAIL: iverilog compilation failed:")
        print(res_cmp.stderr)
        return

    # 4. Run Simulation
    print("[4/4] Executing Physics Simulation (vvp)...")
    res_sim = subprocess.run(["vvp", "sim_twin"], cwd=SIM_DIR, capture_output=True, text=True)
    
    # 5. Extract Result
    print("\n--- Simulation Output ---")
    print(res_sim.stdout)
    
    if res_sim.returncode != 0:
        print("\n--- Simulation Errors ---")
        print(res_sim.stderr)

    if "FINAL_STATE" in res_sim.stdout:
        print("\n>>> SUCCESS: Digital Twin output captured.")
    else:
        print("\n>>> FAIL: No final state found in output.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 benchmarks/run_digital_twin.py <file.thermo>")
        sys.exit(1)
    run_twin(sys.argv[1])