def generate_star_thermo(filename="examples/star_graph.thermo", n_spins=50):
    print(f"Generating Star Graph {filename} with {n_spins} spins...")
    with open(filename, "w") as f:
        f.write(f"// Star Graph (Center=s0 connected to all)\n")
        f.write("energy fn star_model(")
        params = [f"s{i}: float" for i in range(n_spins)]
        f.write(", ".join(params))
        f.write(") -> float {\n")
        
        f.write("    let E = 0.0;\n")
        # Center is s0
        # Leaves are s1...sN
        for i in range(1, n_spins):
            # Ferromagnetic coupling: Center wants to align with leaves
            # But we add frustration by making leaves hate each other
            f.write(f"    E = E + -1.0 * s0 * s{i};  // Center connection\n")
            if i < n_spins - 1:
                # Frustration between leaves
                f.write(f"    E = E + 0.5 * s{i} * s{i+1}; // Leaf frustration\n")
        
        f.write("    return E;\n")
        f.write("}\n\n")
        
        f.write("fn main() -> void {\n")
        f.write("    let res = thermal_anneal(star_model, 5.0, 0.98, 2000);\n")
        f.write("}\n")

if __name__ == "__main__":
    generate_star_thermo()