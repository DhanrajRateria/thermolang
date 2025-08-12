import subprocess
import time
import os
import platform
import re
import json
import numpy as np
from pathlib import Path

# --- Configuration ---
# Use Path for cross-platform compatibility
ROOT_DIR = Path(__file__).parent.parent
COMPILER_PATH = ROOT_DIR / "build" / "thermolangc"
EXAMPLE_FILE = ROOT_DIR / "examples" / "4_domain_specific" / "3_ising_solver.thermo"
FPGA_DIR = ROOT_DIR / "hardware" / "fpga"

def calculate_ising_energy(state, J, h):
    """Calculates the energy of a given spin state for a given Ising model."""
    state = np.array(state)
    J = np.array(J)
    h = np.array(h)
    
    coupling_energy = -0.5 * np.dot(state.T, np.dot(J, state))
    field_energy = -np.dot(h.T, state)
    
    return coupling_energy + field_energy

def parse_final_state(output):
    """Parses the machine-readable final state from a simulation's output."""
    match = re.search(r"\[FINAL_STATE\]: (\[.*\])", output)
    if match:
        try:
            state_str = match.group(1)
            return json.loads(state_str)
        except (json.JSONDecodeError, IndexError):
            return None
    return None

def run_python_backend():
    """Runs the Python simulation backend and returns the final state."""
    print("\n--- Running Python Backend ---")
    output_file = Path(EXAMPLE_FILE.stem + "_sim.py")
    compile_cmd = [str(COMPILER_PATH), str(EXAMPLE_FILE), "--target=sim"]
    
    try:
        subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
        
        with open(output_file, "a") as f:
            f.write("\nif __name__ == '__main__':\n    main()\n")
            
        run_cmd = ["python3", str(output_file)]
        result = subprocess.run(run_cmd, check=True, capture_output=True, text=True, timeout=30)
        
        print(result.stdout)
        return parse_final_state(result.stdout)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
        print("Python backend failed.")
        print("STDOUT:", e.stdout)
        print("STDERR:", e.stderr)
        return None
    finally:
        if output_file.exists():
            output_file.unlink()

def run_cpp_backend():
    """Runs the C++ SPU Simulator backend and returns the final state."""
    print("\n--- Running C++ SPU Simulator Backend ---")
    cpp_file = Path(EXAMPLE_FILE.stem + "_spu.cpp")
    exe_file = Path(ROOT_DIR / (EXAMPLE_FILE.stem + "_spu_exec")) # Place executable in root to avoid path issues
    
    simulator_source_file = ROOT_DIR / "src" / "hardware" / "SPUSimulator.cpp"

    compile_cmd = [str(COMPILER_PATH), str(EXAMPLE_FILE), "--target=spu"]

    try:
        # Generate the C++ file from ThermoLang source
        subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
        
        # Compile the generated C++ code AND the simulator implementation together.
        # This is a much more robust approach than relying on library linking.
        gpp_cmd = [
            "g++", "-std=c++17",
            "-I", str(ROOT_DIR / "include"),  # Include path for headers
            str(cpp_file),                    # The generated main file
            str(simulator_source_file),       # The simulator implementation
            "-o", str(exe_file)
        ]
        subprocess.run(gpp_cmd, check=True, capture_output=True, text=True)
        
        # Run the compiled executable
        result = subprocess.run([str(exe_file)], check=True, capture_output=True, text=True, timeout=30)
        
        print(result.stdout)
        return parse_final_state(result.stdout)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as e:
        print("C++ backend failed.")
        if hasattr(e, 'stdout'):
            print("STDOUT:", e.stdout)
            print("STDERR:", e.stderr)
        return None
    finally:
        if cpp_file.exists(): cpp_file.unlink()
        if exe_file.exists(): exe_file.unlink()

def run_verilog_backend():
    """Runs the Verilog hardware simulation and returns the final state."""
    print("\n--- Running Verilog FPGA Backend ---")
    # Step 1: Generate config files
    config_mem = Path(EXAMPLE_FILE.stem + "_config.mem")
    schedule_txt = Path(EXAMPLE_FILE.stem + "_schedule.txt")
    
    try:
        compile_cmd = [str(COMPILER_PATH), str(EXAMPLE_FILE), "--target=fpga"]
        subprocess.run(compile_cmd, check=True, capture_output=True, text=True)
        
        # Step 2: Run Vivado simulation in batch mode
        # This requires Vivado to be in the system's PATH
        # Create a tcl script to automate the simulation
        tcl_script_path = FPGA_DIR / "run_sim.tcl"
        with open(tcl_script_path, "w") as f:
            f.write(f"set_property top tb_spu_array [get_filesets sim_1]\n")
            f.write(f"launch_simulation\n")
            f.write(f"run all\n")
            f.write(f"exit\n")
        
        vivado_cmd = [
            "vivado", "-mode", "batch", "-source", str(tcl_script_path),
            "-log", "vivado.log", "-journal", "vivado.jou"
        ]
        print(f"Executing Vivado: {' '.join(vivado_cmd)}")
        # Note: Vivado can be slow, so we use a longer timeout.
        result = subprocess.run(vivado_cmd, check=True, capture_output=True, text=True, cwd=FPGA_DIR, timeout=300)
        
        # Step 3: Parse the log file for the final state
        sim_log_path = FPGA_DIR / "vivado.log"
        if sim_log_path.exists():
            with open(sim_log_path, "r") as f:
                log_content = f.read()
            print("Vivado simulation completed. Parsing log...")
            return parse_final_state(log_content)
        return None

    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, FileNotFoundError) as e:
        print("Verilog backend failed. Ensure Vivado is installed and in your PATH.")
        if isinstance(e, FileNotFoundError):
             print("Command 'vivado' not found.")
        elif hasattr(e, 'stdout'):
            print("STDOUT:", e.stdout)
            print("STDERR:", e.stderr)
        return None
    finally:
        # Clean up generated files
        if config_mem.exists(): config_mem.rename(FPGA_DIR / config_mem.name)
        if schedule_txt.exists(): schedule_txt.rename(FPGA_DIR / schedule_txt.name)
        if tcl_script_path.exists(): tcl_script_path.unlink()


def main():
    """Main benchmark orchestration function."""
    print("===== ThermoLang End-to-End Benchmark =====")
    
    if not COMPILER_PATH.exists():
        print(f"Error: Compiler not found at '{COMPILER_PATH}'.")
        print("Please build the project first ('cmake --build build').")
        return
        
    # Define the problem manually to calculate energy
    # From 3_ising_solver.thermo
    J_matrix = [[0, 1, 0, 1], [1, 0, 1, 0], [0, 1, 0, 1], [1, 0, 1, 0]]
    h_vector = [0.5, 0.5, 0.5, 0.5]
    
    results = {}
    results["Python"] = run_python_backend()
    results["C++ SPU Sim"] = run_cpp_backend()
    results["Verilog FPGA"] = run_verilog_backend()
    
    print("\n\n" + "="*50)
    print("          Benchmark Comparison Results")
    print("="*50)
    
    for backend, state in results.items():
        if state and len(state) == 16:
            energy = calculate_ising_energy(state, J_matrix, h_vector)
            print(f"Backend: {backend:<15} | Energy: {energy:8.4f} | Final State: {state}")
        else:
            print(f"Backend: {backend:<15} | FAILED TO PRODUCE RESULT")
            
    print("="*50)

if __name__ == "__main__":
    main()