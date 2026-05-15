import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parent.parent
RTL_DIR = ROOT_DIR / "hardware" / "rtl"
SIM_DIR = ROOT_DIR / "hardware" / "sim"
BUILD_DIR = ROOT_DIR / "build"
OUT_DIR = ROOT_DIR / "out" / "fpga"


def parse_final_state(output_text: str):
    match = re.search(r"\[FINAL_STATE\]:\s*\[([^\]]+)\]", output_text)
    if not match:
        return None
    raw = match.group(1).strip()
    if not raw:
        return []
    return [int(x.strip()) for x in raw.split(",") if x.strip()]


def run_twin(thermo_file: str, manual_config: str = None, manual_schedule: str = None):
    thermo_path = Path(thermo_file).resolve()
    if not thermo_path.exists():
        print(f">>> FAIL: Input file not found: {thermo_path}")
        return None

    print(f"--- Running Digital Twin for {thermo_file} ---")

    compiler = BUILD_DIR / "thermolangc"
    if not compiler.exists():
        print(f">>> FAIL: Compiler not found at {compiler}")
        return None

    SIM_DIR.mkdir(parents=True, exist_ok=True)

    base_name = thermo_path.stem
    out_dir = OUT_DIR / base_name
    out_dir.mkdir(parents=True, exist_ok=True)

    sim_exe = SIM_DIR / "sim_twin.vvp"
    sim_config = SIM_DIR / "config.mem"
    sim_schedule = SIM_DIR / "schedule.txt"

    for path in [sim_exe, sim_config, sim_schedule]:
        if path.exists():
            path.unlink()

    if manual_config and manual_schedule:
        manual_config_path = Path(manual_config).resolve()
        manual_schedule_path = Path(manual_schedule).resolve()

        if not manual_config_path.exists():
            print(f">>> FAIL: Manual config not found: {manual_config_path}")
            return None
        if not manual_schedule_path.exists():
            print(f">>> FAIL: Manual schedule not found: {manual_schedule_path}")
            return None

        shutil.copy2(manual_config_path, out_dir / "config.mem")
        shutil.copy2(manual_schedule_path, out_dir / "schedule.txt")
        shutil.copy2(out_dir / "config.mem", sim_config)
        shutil.copy2(out_dir / "schedule.txt", sim_schedule)
        print(f"[1/4] Using manual config: {manual_config_path.name}")
        print(f"[2/4] Manual config files copied to {SIM_DIR} and saved in {out_dir}")

    else:
        print(f"[1/4] Compiling {thermo_file}...")
        try:
            subprocess.run([str(compiler), str(thermo_path), "--target=fpga"], check=True, cwd=ROOT_DIR)
        except subprocess.CalledProcessError:
            print(">>> FAIL: Compilation failed.")
            return None

        generated_config = ROOT_DIR / f"{base_name}_config.mem"
        generated_schedule = ROOT_DIR / f"{base_name}_schedule.txt"

        if not generated_config.exists() or not generated_schedule.exists():
            print(f">>> FAIL: Compiler did not produce expected {generated_config.name} / {generated_schedule.name}")
            return None

        shutil.move(str(generated_config), str(out_dir / "config.mem"))
        shutil.move(str(generated_schedule), str(out_dir / "schedule.txt"))

        shutil.copy2(out_dir / "config.mem", sim_config)
        shutil.copy2(out_dir / "schedule.txt", sim_schedule)
        print(f"[2/4] Config files copied to {SIM_DIR} and saved in {out_dir}")

    print("[3/4] Compiling Verilog Testbench...")
    iverilog_cmd = [
        "iverilog",
        "-g2012",
        "-DSIMULATION",
        "-I", str(RTL_DIR),
        "-o", str(sim_exe),
        str(SIM_DIR / "tb_spu_array.v"),
        str(RTL_DIR / "spu_array_4x4.v"),
        str(RTL_DIR / "spu_core.v"),
        str(RTL_DIR / "trng.v"),
        str(RTL_DIR / "ring_oscillator.v"),
    ]

    res_cmp = subprocess.run(iverilog_cmd, capture_output=True, text=True)
    if res_cmp.returncode != 0:
        print(">>> FAIL: iverilog compilation failed:")
        print(res_cmp.stderr)
        return None

    print("[4/4] Executing Physics Simulation (vvp)...")
    res_sim = subprocess.run(["vvp", "sim_twin.vvp"], cwd=SIM_DIR, capture_output=True, text=True)

    run_log = out_dir / "run.log"
    run_log.write_text(
        "=== STDOUT ===\n" + res_sim.stdout + "\n=== STDERR ===\n" + res_sim.stderr,
        encoding="utf-8"
    )

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

    if "checkerboard" in thermo_path.name.lower():
        if all(x == final_state[0] for x in final_state):
            print(">>> WARNING: checkerboard case collapsed to a uniform state; treat this as NOT VALIDATED.")

    return final_state


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python3 benchmarks/run_digital_twin.py <file.thermo>")
        print("  python3 benchmarks/run_digital_twin.py <file.thermo> <manual_config.mem> <manual_schedule.txt>")
        sys.exit(1)

    if len(sys.argv) == 4:
        result = run_twin(sys.argv[1], sys.argv[2], sys.argv[3])
    else:
        result = run_twin(sys.argv[1])

    sys.exit(0 if result is not None else 1)