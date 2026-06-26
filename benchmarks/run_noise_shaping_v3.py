import subprocess
import re
import numpy as np
import os
import sys
import matplotlib.pyplot as plt

# Setup
THERMO_FILE = "examples/dense_40.thermo"
COMPILER = "./build/thermolangc"

def calculate_true_energy(spins, J, h):
    n_logical = len(h)
    # Trim padding if necessary
    s = np.array(spins[:n_logical]) if len(spins) > n_logical else np.array(spins)
    energy = -0.5 * s.T @ J @ s - h.T @ s
    return energy

def run_simulation(mode, variance_data=None):
    """Runs compile + sim cycle, returns (energy, variances_str)"""
    env = os.environ.copy()
    env["NOISE_SHAPING_MODE"] = mode
    env["NOISE_SHAPING_VARIANCE_SHRINK"] = "0.5"
    
    # Inject variance data if available (Feedback Loop)
    if variance_data:
        env["NOISE_SHAPING_VARIANCES"] = variance_data

    # 1. Compile
    try:
        subprocess.run([COMPILER, THERMO_FILE, "--target=thrml"], 
                       env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        return None, None

    # 2. Run
    py_file = os.path.basename(THERMO_FILE).replace(".thermo", "_thrml.py")
    res = subprocess.run(["python3", py_file], capture_output=True, text=True)
    
    # 3. Parse
    state_match = re.search(r"\[FINAL_STATE\]:\s*\[(.*?)\]", res.stdout)
    var_match = re.search(r"\[VARIANCES\]:\s*([0-9\.,]+)", res.stdout)
    
    spins = []
    variances_str = ""
    
    if state_match:
        spins = [int(x) for x in state_match.group(1).split(',')]
    
    if var_match:
        variances_str = var_match.group(1)
        
    return spins, variances_str

def run_experiment():
    print("--- ThermoLang Noise Shaping: Feedback Loop Benchmark ---")
    
    # 1. Generate Problem
    subprocess.run(["python3", "benchmarks/generate_dense.py"], check=True)
    
    # 2. Load Ground Truth
    data = np.load(THERMO_FILE.replace(".thermo", ".npz"))
    J_true = data['J']
    h_true = data['h']
    
    results_energy = {}
    
    print(f"\n{'STEP':<20} | {'TRUE ENERGY':<15} | {'NOTES'}")
    print("-" * 60)

    # --- PHASE 1: BASELINE (OFF) ---
    spins_off, var_data = run_simulation("off")
    if spins_off:
        e_off = calculate_true_energy(spins_off, J_true, h_true)
        results_energy["off"] = e_off
        print(f"{'1. Baseline (OFF)':<20} | {e_off:<15.4f} | Profiling variances...")
    else:
        print("Baseline failed.")
        return

    # --- PHASE 2: STATIC ANALYSIS (DEGREE) ---
    spins_deg, _ = run_simulation("degree")
    if spins_deg:
        e_deg = calculate_true_energy(spins_deg, J_true, h_true)
        results_energy["degree"] = e_deg
        print(f"{'2. Static (DEGREE)':<20} | {e_deg:<15.4f} | Heuristic optimization")

    # --- PHASE 3: DYNAMIC PROFILING (VARIANCE) ---
    # We use the 'var_data' captured from Step 1
    spins_var, _ = run_simulation("variance", variance_data=var_data)
    if spins_var:
        e_var = calculate_true_energy(spins_var, J_true, h_true)
        results_energy["variance"] = e_var
        print(f"{'3. Dynamic (VAR)':<20} | {e_var:<15.4f} | Used profile from Step 1")

    # --- PHASE 4: COMBINED ---
    spins_both, _ = run_simulation("degree+variance", variance_data=var_data)
    if spins_both:
        e_both = calculate_true_energy(spins_both, J_true, h_true)
        results_energy["combined"] = e_both
        print(f"{'4. Combined':<20} | {e_both:<15.4f} | Degree + Profile")

    # --- Plotting ---
    modes = list(results_energy.keys())
    energies = list(results_energy.values())
    
    plt.figure(figsize=(10, 6))
    bars = plt.bar(modes, energies, color=['gray', 'skyblue', 'salmon', 'gold'])
    plt.title(f"Impact of Feedback-Driven Noise Shaping (N=40)")
    plt.ylabel("System Energy (Lower is Better)")
    plt.grid(axis='y', alpha=0.3)
    
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height, f'{height:.1f}', ha='center', va='bottom')
        
        os.makedirs("artifacts/generated/benchmarks", exist_ok=True)
        plt.savefig("artifacts/generated/benchmarks/noise_shaping_feedback.png")
        print(f"\n[Artifact] Saved plot to artifacts/generated/benchmarks/noise_shaping_feedback.png")

if __name__ == "__main__":
    run_experiment()