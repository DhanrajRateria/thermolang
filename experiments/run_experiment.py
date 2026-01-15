import subprocess
import re
import json
import time
import os
from datetime import datetime

# Configuration
RESULTS_DIR = "results"
LOG_FILE = os.path.join(RESULTS_DIR, "benchmark_history.json")

def parse_simulation_output(stdout):
    """Extracts physics metrics from Verilog output."""
    data = {}
    
    # Extract Final State
    state_match = re.search(r"\[FINAL_STATE\]:\s*\[(.*?)\]", stdout)
    if state_match:
        # Convert "-1" and "1" strings to integers
        state_str = state_match.group(1)
        data["final_state"] = [int(x) for x in state_str.split(",")]
        
        # Calculate alignment (Magnetization)
        # Perfect order is abs(sum) = 16 (all 1s or all -1s)
        magnetization = sum(data["final_state"])
        data["order_parameter"] = abs(magnetization) / len(data["final_state"])
    
    # Extract Timing
    time_match = re.search(r"Simulation complete", stdout)
    if time_match:
        data["status"] = "Success"
        
    return data

def run_experiment():
    print(f"--- Running ThermoLang Hardware Experiment ---")
    timestamp = datetime.now().isoformat()
    
    # 1. Clean and Rebuild
    print("[1/3] Compiling C++ Compiler...")
    subprocess.run(["make", "compiler"], check=True, capture_output=True)
    
    # 2. Generate Hardware Config
    print("[2/3] Synthesizing Ising Model to FPGA Config...")
    ex_file = "examples/4_domain_specific/3_ising_solver.thermo"
    subprocess.run(["make", "gen-config", f"EXAMPLE={ex_file}"], check=True, capture_output=True)
    
    # 3. Run Verilog Simulation
    print("[3/3] Simulating SPU Physics (iverilog)...")
    start_time = time.time()
    result = subprocess.run(["make", "simulate"], capture_output=True, text=True)
    duration = time.time() - start_time
    
    # 4. Process Results
    metrics = parse_simulation_output(result.stdout)
    metrics["timestamp"] = timestamp
    metrics["duration_seconds"] = duration
    metrics["backend"] = "FPGA_Digital_Twin"
    
    print(f"\n>>> Experiment Complete.")
    print(f"    Final Order Parameter: {metrics.get('order_parameter', 'N/A')} (1.0 = Perfect Crystal)")
    print(f"    Simulation Time:       {duration:.2f}s")
    
    # 5. Save to History
    os.makedirs(RESULTS_DIR, exist_ok=True)
    
    history = []
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE, "r") as f:
            try:
                history = json.load(f)
            except:
                pass
    
    history.append(metrics)
    
    with open(LOG_FILE, "w") as f:
        json.dump(history, f, indent=4)
        
    print(f"    Saved results to {LOG_FILE}")

if __name__ == "__main__":
    run_experiment()