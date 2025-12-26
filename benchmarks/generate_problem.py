import random

def generate_dense_thermo(filename="examples/random_dense.thermo", n_spins=20, density=0.6):
    """
    Generates a dense random Ising model.
    High density creates high-degree nodes, making Noise Shaping effective.
    """
    print(f"Generating {filename} with {n_spins} spins and {density} density...")
    
    with open(filename, "w") as f:
        f.write(f"// Random Dense Ising Model (N={n_spins}, Density={density})\n")
        f.write("// Generated for Noise Shaping Benchmark\n\n")
        
        # Define function signature
        params = [f"s{i}: float" for i in range(n_spins)]
        f.write("energy fn dense_model(\n    ")
        f.write(",\n    ".join(params))
        f.write("\n) -> float {\n")
        
        f.write("    let E = 0.0;\n")
        
        # Generate Couplings
        count = 0
        for i in range(n_spins):
            # Random local field
            h = round(random.uniform(-1.0, 1.0), 2)
            if h != 0:
                 f.write(f"    E = E + {h} * s{i};\n")

            for j in range(i + 1, n_spins):
                if random.random() < density:
                    # Random coupling J
                    J = round(random.uniform(-2.0, 2.0), 2)
                    f.write(f"    E = E + {J} * s{i} * s{j};\n")
                    count += 1
        
        f.write("    // Return NEGATIVE energy because hardware minimizes -E\n")
        f.write("    return -1.0 * E;\n")
        f.write("}\n\n")
        
        f.write("fn main() -> void {\n")
        # Standard annealing schedule
        f.write("    let res = thermal_anneal(dense_model, 10.0, 0.95, 2000);\n")
        f.write("}\n")
    
    print(f"Done. Generated {count} couplings.")

if __name__ == "__main__":
    generate_dense_thermo()