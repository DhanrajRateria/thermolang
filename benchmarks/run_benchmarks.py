import subprocess
import time
import os
import platform

# --- Configuration ---
COMPILER_PATH = "../build/thermolangc"
EXAMPLE_FILE = "../examples/4_domain_specific/3_ising_solver.thermo"
OUTPUT_PY_FILE = "program_sim.py"
PYTHON_INTERPRETER = "python3" if platform.system() != "Windows" else "python"

def compile_and_run(use_optimizations: bool):
    """Compiles the source file and runs the resulting python script, returning execution time."""
    
    print("-" * 50)
    if use_optimizations:
        print("Running WITH Optimizations...")
        compile_command = [COMPILER_PATH, EXAMPLE_FILE, "--target=sim"]
    else:
        print("Running WITHOUT Optimizations...")
        compile_command = [COMPILER_PATH, EXAMPLE_FILE, "--target=sim", "--no-opts"]

    # 1. Compile the ThermoLang code
    try:
        print(f"Executing: {' '.join(compile_command)}")
        subprocess.run(compile_command, check=True, capture_output=True, text=True)
    except subprocess.CalledProcessError as e:
        print("COMPILATION FAILED.")
        print("STDOUT:", e.stdout)
        print("STDERR:", e.stderr)
        return float('inf') # Return infinity on failure

    # Add a main execution block to the generated python script
    with open(OUTPUT_PY_FILE, "a") as f:
        f.write("\n\nif __name__ == '__main__':\n")
        f.write("    main()\n")

    # 2. Run the generated Python script and time it
    start_time = time.perf_counter()
    try:
        run_command = [PYTHON_INTERPRETER, OUTPUT_PY_FILE]
        subprocess.run(run_command, check=True, capture_output=True, text=True)
    except subprocess.CalledProcessError as e:
        print("PYTHON EXECUTION FAILED.")
        print("STDOUT:", e.stdout)
        print("STDERR:", e.stderr)
        return float('inf')
    end_time = time.perf_counter()
    
    execution_time = end_time - start_time
    print(f"Execution Time: {execution_time:.4f} seconds")
    
    # Clean up the generated file
    os.remove(OUTPUT_PY_FILE)
    
    return execution_time

def main():
    """Main benchmark function."""
    print("===== ThermoLang Optimization Benchmark =====")
    
    if not os.path.exists(COMPILER_PATH):
        print(f"Error: Compiler not found at '{COMPILER_PATH}'.")
        print("Please build the project first (e.g., run 'cmake --build build' in the root directory).")
        return

    time_with_opts = compile_and_run(use_optimizations=True)
    time_without_opts = compile_and_run(use_optimizations=False)

    print("-" * 50)
    print("\n===== Benchmark Results =====")
    if time_with_opts == float('inf') or time_without_opts == float('inf'):
        print("Benchmark failed due to an error in one of the runs.")
    else:
        speedup = time_without_opts / time_with_opts
        print(f"Optimized Run Time:   {time_with_opts:.4f}s")
        print(f"Unoptimized Run Time: {time_without_opts:.4f}s")
        print(f"Speedup: {speedup:.2f}x")
    print("=" * 29)

if __name__ == "__main__":
    main()