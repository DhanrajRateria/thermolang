def generate_teleport_hard(filename="examples/teleport_hard.thermo", n_spins=64):
    print(f"Generating Long-Distance Connection {filename}...")
    with open(filename, "w") as f:
        f.write(f"// Teleportation Demo (Corner to Corner)\n")
        f.write(f"// We define {n_spins} variables to fill the 8x8 grid.\n")
        f.write("energy fn teleport_hard(")
        
        # Declare all 64 variables to force placement logic
        params = [f"s{i}: float" for i in range(n_spins)]
        f.write(", ".join(params))
        f.write(") -> float {\n")
        
        # Only couple the first and the last
        # On an 8x8 grid, s0 is (0,0) and s63 is (7,7).
        # Distance = |7-0| + |7-0| = 14 hops.
        f.write(f"    // Connect Top-Left (s0) to Bottom-Right (s{n_spins-1})\n")
        f.write(f"    let E = -1.0 * s0 * s{n_spins-1};\n")
        
        f.write("    return E;\n")
        f.write("}\n\n")
        
        f.write("fn main() -> void {\n")
        f.write(f"    let res = thermal_anneal(teleport_hard, 5.0, 0.99, 1000);\n")
        f.write("}\n")

if __name__ == "__main__":
    generate_teleport_hard()