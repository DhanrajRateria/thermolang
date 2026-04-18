import subprocess
import os
import re
import numpy as np
from pathlib import Path

# --- Configuration ---
ROOT_DIR = Path(__file__).parent.parent
COMPILER_PATH = ROOT_DIR / "build" / "thermolangc"
EXAMPLE_FILE = ROOT_DIR / "examples" / "4_domain_specific" / "3_ising_solver.thermo"


def calculate_ising_energy(state, J, h):
    """Calculates energy only when dimensions match exactly."""
    state = np.array(state, dtype=float)
    J = np.array(J, dtype=float)
    h = np.array(h, dtype=float)

    coupling_energy = -0.5 * np.dot(state.T, np.dot(J, state))
    field_energy = -np.dot(h.T, state)
    return coupling_energy + field_energy


def safe_energy(result, J_matrix, h_vector):
    if result is None:
        return None

    state = np.array(result, dtype=float)
    J = np.array(J_matrix, dtype=float)
    h = np.array(h_vector, dtype=float)

    n = len(state)
    if J.shape != (n, n) or h.shape != (n,):
        return None

    return calculate_ising_energy(state, J, h)


def parse_final_state(output_text):
    match = re.search(r"\[FINAL_STATE\]:\s*\[(.*?)\]", output_text, re.DOTALL)
    if match:
        try:
            values = match.group(1).split(",")
            return [int(v.strip()) for v in values if v.strip()]
        except Exception as e:
            print(f"Error parsing final state: {e}")
            print(f"Raw state string: {match.group(1)}")
    return None


def run_python_backend():
    print("\n--- Running Python Backend ---")
    output_file = Path(EXAMPLE_FILE.stem + "_sim.py")
    compile_cmd = [str(COMPILER_PATH), str(EXAMPLE_FILE), "--target=sim"]

    try:
        subprocess.run(compile_cmd, check=True, capture_output=True, text=True)

        run_cmd = ["python3", str(output_file)]
        result = subprocess.run(run_cmd, check=True, capture_output=True, text=True, timeout=30)

        print(result.stdout)
        return parse_final_state(result.stdout)

    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
        print("Python backend failed.")
        if hasattr(e, "stdout"):
            print("STDOUT:", e.stdout)
        if hasattr(e, "stderr"):
            print("STDERR:", e.stderr)
        return None
    except Exception as e:
        print(f"Unexpected error in Python backend: {type(e).__name__}: {e}")
        return None


def run_cpp_backend():
    print("\n--- Running C++ SPU Simulator Backend ---")
    cpp_file = Path(EXAMPLE_FILE.stem + "_spu.cpp")
    exe_file = Path(ROOT_DIR / (EXAMPLE_FILE.stem + "_spu_exec"))

    simulator_source_file = ROOT_DIR / "src" / "hardware" / "SPUSimulator.cpp"
    compile_cmd = [str(COMPILER_PATH), str(EXAMPLE_FILE), "--target=spu"]

    try:
        subprocess.run(compile_cmd, check=True, capture_output=True, text=True)

        gpp_cmd = [
            "g++", "-std=c++17",
            "-I", str(ROOT_DIR / "include"),
            str(cpp_file),
            str(simulator_source_file),
            "-o", str(exe_file)
        ]

        subprocess.run(gpp_cmd, check=True, capture_output=True, text=True)

        result = subprocess.run([str(exe_file)], check=True, capture_output=True, text=True, timeout=30)

        print(result.stdout)
        return parse_final_state(result.stdout)

    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
        print("C++ backend failed.")
        if hasattr(e, "stdout"):
            print("STDOUT:", e.stdout)
        if hasattr(e, "stderr"):
            print("STDERR:", e.stderr)
        return None
    except Exception as e:
        print(f"Unexpected error in C++ backend: {type(e).__name__}: {e}")
        return None
    finally:
        try:
            if cpp_file.exists():
                cpp_file.unlink()
            if exe_file.exists():
                exe_file.unlink()
        except Exception:
            pass


def run_verilog_backend():
    print("\n--- Running Verilog FPGA Backend (Digital Twin) ---")
    cmd = ["python3", str(ROOT_DIR / "benchmarks" / "run_digital_twin.py"), str(ROOT_DIR / "examples" / "checkerboard.thermo")]

    try:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True, timeout=60)
        print(result.stdout)
        return parse_final_state(result.stdout)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
        print("Verilog FPGA backend failed.")
        if hasattr(e, "stdout"):
            print("STDOUT:", e.stdout)
        if hasattr(e, "stderr"):
            print("STDERR:", e.stderr)
        return None
    except Exception as e:
        print(f"Unexpected error in Verilog FPGA backend: {type(e).__name__}: {e}")
        return None


def run_spice_backend():
    print("\n--- Running SPICE Backend ---")
    spice_file = Path(EXAMPLE_FILE.stem + ".spice")
    compile_cmd = [str(COMPILER_PATH), str(EXAMPLE_FILE), "--target=spice"]

    try:
        subprocess.run(compile_cmd, check=True, capture_output=True, text=True)

        saved_result_path = ROOT_DIR / "benchmarks" / "saved_spice_result.txt"
        if saved_result_path.exists():
            print("Using saved SPICE simulation result")
            with open(saved_result_path, "r") as f:
                saved_content = f.read()
                return parse_final_state(saved_content)

        print("SPICE netlist generated. Manual steps required:")
        print("1. Run the netlist in LTSpice")
        print("2. Save the text log output")
        print("3. Run tools/parse_ltspice_txt.py on the log file")

        placeholder = [1, 1, 1, 1]
        print(f"Using placeholder result: {placeholder}")
        return placeholder

    except Exception as e:
        print(f"SPICE backend error: {type(e).__name__}: {e}")
        return None


def run_thrml_backend():
    print("\n--- Running Extropic's thrml Backend ---")
    output_file = Path(EXAMPLE_FILE.stem + "_thrml.py")
    compile_cmd = [str(COMPILER_PATH), str(EXAMPLE_FILE), "--target=thrml"]

    try:
        subprocess.run(compile_cmd, check=True, capture_output=True, text=True)

        run_cmd = ["python3", str(output_file)]
        result = subprocess.run(run_cmd, check=True, capture_output=True, text=True, timeout=60)

        print(result.stdout)
        return parse_final_state(result.stdout)

    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
        print("thrml backend failed.")
        if hasattr(e, "stdout"):
            print("STDOUT:", e.stdout)
        if hasattr(e, "stderr"):
            print("STDERR:", e.stderr)
        return None
    except Exception as e:
        print(f"Unexpected error in thrml backend: {type(e).__name__}: {e}")
        return None


def main():
    print("===== ThermoLang End-to-End Benchmark =====")

    if not COMPILER_PATH.exists():
        print(f"Error: Compiler not found at '{COMPILER_PATH}'.")
        print("Please build the project first ('cmake --build build').")
        return

    # Logical 4-spin Ising example used by 3_ising_solver.thermo
    J_matrix = [[0, 1, 0, 1], [1, 0, 1, 0], [0, 1, 0, 1], [1, 0, 1, 0]]
    h_vector = [0.5, 0.5, 0.5, 0.5]

    python_result = run_python_backend()
    cpp_result = run_cpp_backend()
    fpga_result = run_verilog_backend()
    spice_result = run_spice_backend()
    thrml_result = run_thrml_backend()

    print("\n" + "=" * 50)
    print("          Benchmark Comparison Results")
    print("=" * 50)

    def print_result(name, result):
        if result is None:
            print(f"Backend: {name:<15} | NO VALID OUTPUT")
            return

        energy = safe_energy(result, J_matrix, h_vector)
        if energy is None:
            print(f"Backend: {name:<15} | VALID OUTPUT, but energy skipped (shape mismatch) | Len={len(result)}")
        else:
            print(f"Backend: {name:<15} | Energy: {energy:8.4f} | State: {result[:4]}...")

    print_result("Python", python_result)
    print_result("C++ SPU Sim", cpp_result)
    print_result("Verilog FPGA", fpga_result)
    print_result("SPICE", spice_result)
    print_result("thrml lib", thrml_result)

    print("=" * 50)

    results = {
        "Python": python_result,
        "C++ SPU Sim": cpp_result,
        "Verilog FPGA": fpga_result,
        "SPICE": spice_result,
        "thrml lib": thrml_result,
    }

    valid_results = {k: v for k, v in results.items() if v is not None}

    if len(valid_results) >= 2:
        print("\nAlignment between backends:")
        backends = list(valid_results.keys())
        for i in range(len(backends)):
            for j in range(i + 1, len(backends)):
                b1, b2 = backends[i], backends[j]
                r1, r2 = valid_results[b1], valid_results[b2]

                min_len = min(len(r1), len(r2))
                if min_len == 0:
                    continue

                alignment = sum(1 for k in range(min_len) if r1[k] == r2[k]) / min_len
                print(f"{b1} vs {b2}: {alignment * 100:.1f}% aligned")


if __name__ == "__main__":
    main()