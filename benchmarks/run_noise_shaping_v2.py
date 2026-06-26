import subprocess
import re
import numpy as np
import os
import sys
import matplotlib.pyplot as plt

# Setup
THERMO_FILE = "examples/dense_40.thermo"
COMPILER = "./build/thermolangc"
MODES = ["off", "degree", "variance", "degree+variance"]

def calculate_true_energy(spins, J, h):
    # Fix: Slice spins to match problem size if hardware added padding
    n_logical = len(h)
    if len(spins) > n_logical:
        # print(f"  [DEBUG] Trimming hardware output from {len(spins)} to {n_logical}")
        s = np.array(spins[:n_logical])
    else:
        s = np.array(spins)
        
    # E = -0.5 * s.T * J * s - h.T * s
    energy = -0.5 * s.T @ J @ s - h.T @ s
    return energy

def run_experiment():
    print("--- ThermoLang Noise Shaping Benchmark (Rigorous) ---")
    
    # 1. Generate Problem
    subprocess.run(["python3", "benchmarks/generate_dense.py"], check=True)
    
    # 2. Load Ground Truth
    data = np.load(THERMO_FILE.replace(".thermo", ".npz"))
    J_true = data['J']
    h_true = data['h']
    
    results = {}
    
    print(f"\n{'MODE':<20} | {'INTERNAL E':<15} | {'TRUE ENERGY':<15} | {'STATUS'}")
    print("-" * 75)

    for mode in MODES:
        env = os.environ.copy()
        env["NOISE_SHAPING_MODE"] = mode
        env["NOISE_SHAPING_VARIANCE_SHRINK"] = "0.5"
        
        # Compile (suppress output to keep terminal clean)
        try:
            subprocess.run([COMPILER, THERMO_FILE, "--target=thrml"], 
                           env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError:
            print(f"{mode:<20} | {'N/A':<15} | {'N/A':<15} | FAIL (Compile)")
            continue
            
        # Run Simulation
        py_file = os.path.basename(THERMO_FILE).replace(".thermo", "_thrml.py")
        res = subprocess.run(["python3", py_file], capture_output=True, text=True)
        
        # Extract State
        match = re.search(r"\[FINAL_STATE\]:\s*\[(.*?)\]", res.stdout)
        if match:
            state_str = match.group(1)
            spins = [int(x) for x in state_str.split(',')]
            
            # Extract Reported (Internal) Energy
            e_match = re.search(r"\[FINAL_ENERGY\]:\s*([\d\.\-]+)", res.stdout)
            internal_e = float(e_match.group(1)) if e_match else 0.0
            
            # Calculate True Energy
            true_e = calculate_true_energy(spins, J_true, h_true)
            results[mode] = true_e
            
            print(f"{mode:<20} | {internal_e:<15.4f} | {true_e:<15.4f} | OK")
        else:
            print(f"{mode:<20} | {'N/A':<15} | {'N/A':<15} | FAIL (Parse)")
            # print(res.stdout) # Uncomment for debug

    # Conclusion & Plotting
    if results:
        best_mode = min(results, key=results.get)
        baseline = results.get("off", 0)
        improvement = baseline - results[best_mode]
        pct = (improvement / abs(baseline)) * 100 if baseline != 0 else 0
        
        print("-" * 75)
        print(f"Winner: {best_mode.upper()} ({results[best_mode]:.4f})")
        print(f"Improvement: {improvement:.4f} ({pct:.2f}%)")
        
        # Plot
        modes = list(results.keys())
        energies = list(results.values())
        
        plt.figure(figsize=(10, 6))
        bars = plt.bar(modes, energies, color=['gray', 'skyblue', 'lightgreen', 'gold'])
        plt.title(f"Noise Shaping Performance (Lower Energy is Better)\nRandom Dense Graph N=40")
        plt.ylabel("System Energy")
        plt.grid(axis='y', alpha=0.3)
        
        # Add labels
        for bar in bars:
            height = bar.get_height()
            plt.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.1f}', ha='center', va='bottom')
            
        os.makedirs("artifacts/generated/benchmarks", exist_ok=True)
        plt.savefig("artifacts/generated/benchmarks/noise_shaping_final.png")
        print("\n[Artifact] Saved plot to artifacts/generated/benchmarks/noise_shaping_final.png")

if __name__ == "__main__":
    run_experiment()