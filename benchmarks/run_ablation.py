"""
Noise shaping ablation harness.
Generates synthetic Ising problems and runs Metropolis sampling under
multiple beta shaping modes (global, degree, variance, degree+variance).
Outputs JSON/CSV metrics and optional plots.
"""

import argparse
import json
import math
import time
import subprocess
import os
import re
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path

# --- Configuration ---
COMPILER = "./build/thermolangc"
OUT_DIR = Path("benchmarks/results")
OUT_DIR.mkdir(parents=True, exist_ok=True)

# --- Graph Generators ---

def generate_graph(kind: str, n: int, seed: int):
    filename = f"examples/{kind}_{n}.thermo"
    print(f"Generating {kind} graph...")
    rng = np.random.default_rng(seed)
    
    with open(filename, "w") as f:
        f.write(f"// {kind} Graph N={n}\n")
        f.write("energy fn model(" + ", ".join([f"s{i}: float" for i in range(n)]) + ") -> float {\n")
        f.write("    let E = 0.0;\n")
        
        # 1. Dense (Hub & Spoke / Random)
        if kind == "dense":
            for i in range(n):
                for j in range(i+1, n):
                    if rng.random() < 0.4: # 40% density
                        J = rng.normal(0, 1)
                        f.write(f"    E = E + {J:.3f} * s{i} * s{j};\n")

        # 2. Sparse (Erdos-Renyi)
        elif kind == "sparse":
             for i in range(n):
                for j in range(i+1, n):
                    if rng.random() < 0.1: # 10% density
                        J = rng.normal(0, 1)
                        f.write(f"    E = E + {J:.3f} * s{i} * s{j};\n")
        
        # 3. Ring (Uniform Degree = 2) - Control Group
        # Degree shaping should fail here. Variance shaping might help.
        elif kind == "ring":
            for i in range(n):
                j = (i + 1) % n
                J = -1.0 # Ferromagnetic ring
                f.write(f"    E = E + {J:.3f} * s{i} * s{j};\n")

        f.write("    return -1.0 * E;\n")
        f.write("}\n")
        f.write("fn main() -> void {\n")
        f.write(f"    let res = thermal_anneal(model, 5.0, 0.98, 2000);\n")
        f.write("}\n")
    
    return filename

# --- Execution Engine ---

def run_compile_and_sim(source_file, mode, variance_data=None):
    env = os.environ.copy()
    env["NOISE_SHAPING_MODE"] = mode
    env["NOISE_SHAPING_VARIANCE_SHRINK"] = "0.5"
    
    if variance_data:
        env["NOISE_SHAPING_VARIANCES"] = variance_data

    # 1. Compile
    gen_file = source_file.replace("examples/", "").replace(".thermo", "_thrml.py")
    try:
        subprocess.run([COMPILER, source_file, "--target=thrml"], 
                       env=env, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except subprocess.CalledProcessError:
        return None, None

    # 2. Run Sim
    try:
        res = subprocess.run(["python3", gen_file], capture_output=True, text=True)
        
        # DEBUG: Print output if parsing fails
        energy = 0.0
        e_match = re.search(r"\[FINAL_ENERGY\]:\s*([\d\.\-]+)", res.stdout)
        if e_match: 
            energy = float(e_match.group(1))
        else:
            print(f"    ! Failed to parse ENERGY for mode {mode}")
            # print(res.stdout[-200:]) # Uncomment to see tail of output

        variances = ""
        v_match = re.search(r"\[VARIANCES\]:\s*([\d\.,]+)", res.stdout)
        if v_match: 
            variances = v_match.group(1)
        else:
            # If we are in OFF mode, we NEED variances for the next step.
            if mode == "off":
                print(f"    ! Failed to parse VARIANCES for mode {mode}")
        
        return energy, variances
    except Exception as e:
        print(f"    ! Exception: {e}")
        return None, None

# --- Main Ablation Loop ---

def main():
    graphs = ["dense", "sparse", "ring"]
    modes = ["off", "degree", "degree+variance"]
    
    results = {g: {} for g in graphs}

    for g_type in graphs:
        src = generate_graph(g_type, 40, seed=42)
        
        # 1. Baseline (Off)
        e_off, v_off = run_compile_and_sim(src, "off")
        results[g_type]["off"] = e_off
        
        # 2. Degree Only
        e_deg, _ = run_compile_and_sim(src, "degree")
        results[g_type]["degree"] = e_deg
        
        # 3. Variance Tracking (Iterative)
        # Pass 1: Run with Degree mode to get initial variances (or Off mode)
        # We use the variances captured from the "off" run to guide the next run
        if v_off:
            e_var, _ = run_compile_and_sim(src, "degree+variance", variance_data=v_off)
            results[g_type]["iterative_var"] = e_var
        else:
            results[g_type]["iterative_var"] = 0.0

    # --- Plotting ---
    print("\n=== Final Results ===")
    print(json.dumps(results, indent=2))
    
    fig, axes = plt.subplots(1, 3, figsize=(15, 5))
    
    for i, g in enumerate(graphs):
        ax = axes[i]
        data = results[g]
        names = list(data.keys())
        values = list(data.values())
        
        # Normalize so lower is better (more negative energy)
        ax.bar(names, values, color=['gray', 'blue', 'purple'])
        ax.set_title(f"{g.capitalize()} Graph (N=40)")
        ax.set_ylabel("Energy")
        
    plt.tight_layout()
    plt.savefig("benchmarks/ablation_study.png")
    print("Saved plot to benchmarks/ablation_study.png")

if __name__ == "__main__":
    main()