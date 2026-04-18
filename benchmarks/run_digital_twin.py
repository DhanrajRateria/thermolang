import os
import re
import shutil
import subprocess
import sys

RTL_DIR = "hardware/rtl"
SIM_DIR = "hardware/sim"
BUILD_DIR = "build"


def parse_final_state(output_text):
    match = re.search(r"\[FINAL_STATE\]:\s*\[([^\]]+)\]", output_text)
    if not match:
        return None
    raw = match.group(1).strip()
    if not raw:
        return []
    return [int(x.strip()) for x in raw.split(",") if x.strip()]


def run_twin(thermo_file):
    print(f"--- Running Digital Twin for {thermo_file} ---")

    os.makedirs(SIM_DIR, exist_ok=True)

    base_name = os.path.splitext(os.path.basename(thermo_file))[0]
    compiler = f"./{BUILD_DIR}/thermolangc"

    sim_exe = os.path.join(SIM_DIR, "sim_twin.vvp")
    sim_config = os.path.join(SIM_DIR, "config.mem")
    sim_schedule = os.path.join(SIM_DIR, "schedule.txt")

    # Clean stale generated files that may confuse the flow
    for path in [sim_exe, sim_config, sim_schedule]:
        if os.path.exists(path):
            os.remove(path)

    print(f"[1/4] Compiling {thermo_file}...")
    try:
        subprocess.run([compiler, thermo_file, "--target=fpga"], check=True)
    except subprocess.CalledProcessError:
        print(">>> FAIL: Compilation failed.")
        return None

    try:
        shutil.move(f"{base_name}_config.mem", sim_config)
        shutil.move(f"{base_name}_schedule.txt", sim_schedule)
        print(f"[2/4] Config files moved to {SIM_DIR}")
    except FileNotFoundError:
        print(f">>> FAIL: Compiler did not produce expected {base_name}_config.mem / {base_name}_schedule.txt")
        return None

    print("[3/4] Compiling Verilog Testbench...")
    iverilog_cmd = [
        "iverilog",
        "-g2012",
        "-DSIMULATION",
        "-I", RTL_DIR,
        "-o", sim_exe,
        f"{SIM_DIR}/tb_spu_array.v",
        f"{RTL_DIR}/spu_array_4x4.v",
        f"{RTL_DIR}/spu_core.v",
        f"{RTL_DIR}/trng.v",
        f"{RTL_DIR}/ring_oscillator.v",
    ]

    res_cmp = subprocess.run(iverilog_cmd, capture_output=True, text=True)
    if res_cmp.returncode != 0:
        print(">>> FAIL: iverilog compilation failed:")
        print(res_cmp.stderr)
        return None

    print("[4/4] Executing Physics Simulation (vvp)...")
    res_sim = subprocess.run(["vvp", "sim_twin.vvp"], cwd=SIM_DIR, capture_output=True, text=True)

    print("\n--- Simulation Output ---")
    print(res_sim.stdout)

    if res_sim.returncode != 0:
        print("\n--- Simulation Errors ---")
        print(res_sim.stderr)

    final_state = parse_final_state(res_sim.stdout)
    if final_state is None:
        print("\n>>> FAIL: No final state found in output.")
        return None

    print("\n>>> SUCCESS: Digital Twin output captured.")

    if "checkerboard" in os.path.basename(thermo_file).lower():
        if all(x == final_state[0] for x in final_state):
            print(">>> WARNING: checkerboard case collapsed to a uniform state; treat this as NOT VALIDATED.")

    return final_state


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 benchmarks/run_digital_twin.py <file.thermo>")
        sys.exit(1)

    result = run_twin(sys.argv[1])
    sys.exit(0 if result is not None else 1)