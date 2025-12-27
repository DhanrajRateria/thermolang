def generate_k5_thermo(filename="examples/k5_graph.thermo"):
    print(f"Generating Fully Connected K5 Graph {filename}...")
    with open(filename, "w") as f:
        f.write("// K5 Graph (Fully Connected 5 Nodes)\n")
        f.write("// This is non-planar. It REQUIRES routing/chains to exist on a 2D grid.\n")
        
        # 5 spins
        f.write("energy fn k5_model(s0: float, s1: float, s2: float, s3: float, s4: float) -> float {\n")
        f.write("    let E = 0.0;\n")
        
        # All-to-All connections
        for i in range(5):
            for j in range(i+1, 5):
                # Antiferromagnetic frustration
                f.write(f"    E = E + 1.0 * s{i} * s{j};\n")
        
        f.write("    return E;\n")
        f.write("}\n\n")
        
        f.write("fn main() -> void {\n")
        f.write("    let res = thermal_anneal(k5_model, 10.0, 0.95, 1000);\n")
        f.write("}\n")

if __name__ == "__main__":
    generate_k5_thermo()