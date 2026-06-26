import subprocess
import os
import re
import sys
import matplotlib.pyplot as plt

# Configuration
SOURCE_FILE = "examples/star_graph.thermo"
COMPILER = "./build/thermolangc"
PYTHON_GEN_FILE = "star_graph_thrml.py" # Output of compiler

# The 4 Modes of your NoiseShapingPass
MODES = ["off", "degree", "variance", "degree+variance"]

def run_benchmark():
    results = {}
    
    if not os.path.exists(SOURCE_FILE):
        print(f"Error: {SOURCE_FILE} not found. Run generate_problem.py first.")
        sys.exit(1)

    print(f"--- Starting Noise Shaping Benchmark on {SOURCE_FILE} ---")

    for mode in MODES:
        print(f"\n[Experiment] Compiling with Mode: {mode.upper()}")
        
        # 1. Set Compiler Environment Variables
        env = os.environ.copy()
        env["NOISE_SHAPING_MODE"] = mode
        # "Variance" mode needs tracking enabled (usually default in optimizer, but explicit here)
        env["NOISE_SHAPING_VARIANCE_SHRINK"] = "0.5" 

        # 2. Compile
        # This runs your C++ compiler with the specific env var
        try:
            subprocess.run([COMPILER, SOURCE_FILE, "--target=thrml"], 
                         env=env, check=True, stdout=subprocess.PIPE)
        except subprocess.CalledProcessError as e:
            print(f"Compilation failed for mode {mode}")
            continue

        # 3. Run the generated Python/JAX code
        # We capture stdout to read the [FINAL_ENERGY] tag we added in Step 1
        print(f"  > Running JAX Simulation...")
        try:
            # Note: The compiler output filename might vary based on your implementation.
            # Usually it's source_filename + "_thrml.py"
            # Adjust this path if your compiler outputs somewhere else.
            result = subprocess.run(["python3", PYTHON_GEN_FILE], capture_output=True, text=True)
            
            # 4. Parse Energy
            energy_match = re.search(r"\[FINAL_ENERGY\]:\s*([\d\.\-]+)", result.stdout)
            if energy_match:
                energy = float(energy_match.group(1))
                results[mode] = energy
                print(f"  > Final Energy: {energy:.4f}")
            else:
                print("  > Error: Could not parse energy from simulation output.")
                print(result.stdout[-200:]) # Print last few lines for debug

        except Exception as e:
            print(f"  > Simulation failed: {e}")

    return results

def visualize(results):
    if not results:
        print("No results to plot.")
        return

    print("\n--- Summary ---")
    base_energy = results.get("off", 0)
    for mode, energy in results.items():
        diff = energy - base_energy
        pct = (diff / abs(base_energy)) * 100 if base_energy != 0 else 0
        print(f"{mode:<15}: {energy:.4f} (Diff: {diff:.4f}, {pct:.2f}%)")

    # Simple Bar Chart
    modes = list(results.keys())
    energies = list(results.values())

    plt.figure(figsize=(10, 6))
    bars = plt.bar(modes, energies, color=['gray', 'skyblue', 'lightgreen', 'salmon'])
    
    plt.ylabel('Final Energy (Lower is Better)')
    plt.title('ThermoLang Noise Shaping Performance')
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    
    # Add value labels
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.2f}',
                ha='center', va='bottom')

    os.makedirs("artifacts/generated/benchmarks", exist_ok=True)
    plt.savefig("artifacts/generated/benchmarks/noise_shaping_result.png")
    print("\nPlot saved to artifacts/generated/benchmarks/noise_shaping_result.png")

if __name__ == "__main__":
    data = run_benchmark()
    visualize(data)