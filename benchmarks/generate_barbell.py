import os


def generate_barbell(filename="examples/barbell.thermo", n_cluster=10):
    total = n_cluster * 2 + 1 # Left + Right + Bridge
    bridge_idx = n_cluster

    # Ensure output directory exists
    os.makedirs(os.path.dirname(filename), exist_ok=True)

    with open(filename, "w") as f:
        f.write(f"// Barbell Graph: Two clusters connected by s{bridge_idx}\n")
        f.write("energy fn barbell(")
        f.write(", ".join([f"s{i}: float" for i in range(total)]))
        f.write(") -> float {\n")
        f.write("    let E = 0.0;\n")

        # Left Cluster (Ferromagnetic)
        for i in range(n_cluster):
            for j in range(i+1, n_cluster):
                f.write(f"    E = E + -1.0 * s{i} * s{j};\n")

        # Right Cluster (Ferromagnetic, but Anti-aligned to Left)
        # We start Right Cluster at bridge_idx + 1
        start_right = bridge_idx + 1
        for i in range(start_right, total):
            for j in range(i+1, total):
                f.write(f"    E = E + -1.0 * s{i} * s{j};\n")

        # The Bridge
        # Connects to ONE node on left (0) and ONE on right (start_right)
        # This makes it Low Degree (2), but High Variance (Frustrated)
        f.write(f"    E = E + -1.0 * s0 * s{bridge_idx};\n")
        f.write(f"    E = E + -1.0 * s{bridge_idx} * s{start_right};\n")

        f.write("    return E;\n")
        f.write("}\n\n")
        f.write("fn main() -> void {\n")
        f.write("    let res = thermal_anneal(barbell, 5.0, 0.98, 5000);\n")
        f.write("}\n")

    return filename

if __name__ == "__main__":
    generate_barbell()