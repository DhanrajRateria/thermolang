import numpy as np
import os

from generated_artifacts import generated_artifact_path

def generate_dense_problem(filename="examples/dense_40.thermo", n=40, density=0.5):
    print(f"Generating Random Dense Graph (N={n}, Density={density})...")
    rng = np.random.default_rng(42) # Fixed seed for reproducibility
    
    # Generate Symmetric J matrix
    mask = rng.random((n, n)) < density
    mask = np.triu(mask, 1) # Upper triangle only
    J = rng.normal(0, 1, (n, n)) * mask
    J = (J + J.T) # Symmetric
    
    # Generate h vector
    h = rng.normal(0, 0.5, n)
    
    # Save Ground Truth params for the benchmark runner to read later
    np.savez(generated_artifact_path(filename, ".npz"), J=J, h=h)
    
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, "w") as f:
        f.write(f"// Dense Graph N={n}\n")
        params = [f"s{i}: float" for i in range(n)]
        f.write(f"energy fn model({', '.join(params)}) -> float {{\n")
        f.write("    let E = 0.0;\n")
        
        # Write couplings
        for i in range(n):
            if h[i] != 0:
                f.write(f"    E = E + {h[i]:.3f} * s{i};\n")
            for j in range(i+1, n):
                if J[i, j] != 0:
                    f.write(f"    E = E + {J[i, j]:.3f} * s{i} * s{j};\n")
                    
        f.write("    return -1.0 * E;\n") # Maximize stability (minimize energy)
        f.write("}\n")
        f.write("fn main() -> void {\n")
        f.write("    let res = thermal_anneal(model, 5.0, 0.98, 2000);\n")
        f.write("}\n")

if __name__ == "__main__":
    generate_dense_problem()