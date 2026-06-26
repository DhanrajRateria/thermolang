import numpy as np
import os

from generated_artifacts import generated_artifact_path

def generate_regular_varmix(filename="examples/regular_varmix_40.thermo", n=40, k=6):
    """
    k-regular-ish graph (each node connects to k neighbors in a ring+chords pattern),
    but edge magnitudes are bimodal (strong/weak), creating variance heterogeneity
    while keeping degrees uniform.
    """
    rng = np.random.default_rng(123)

    # Build a structured k-regular connectivity: ring + offsets
    offsets = [1, 2, 5]  # gives degree 6 (± each offset)
    assert 2 * len(offsets) == k

    J = np.zeros((n, n), dtype=np.float32)

    # Assign bimodal magnitudes
    strong = 2.5
    weak = 0.3

    for i in range(n):
        for off in offsets:
            for j in [(i + off) % n, (i - off) % n]:
                # choose strong/weak randomly per edge but symmetrically
                mag = strong if rng.random() < 0.5 else weak
                sign = 1.0 if rng.random() < 0.5 else -1.0
                J[i, j] = sign * mag

    # symmetrize and zero diagonal
    J = np.triu(J, 1)
    J = J + J.T

    # small random fields
    h = rng.normal(0, 0.2, n).astype(np.float32)

    np.savez(generated_artifact_path(filename, ".npz"), J=J, h=h)
    os.makedirs(os.path.dirname(filename), exist_ok=True)

    with open(filename, "w") as f:
        f.write(f"// Regular VarMix N={n}, k={k}\n")
        params = [f"s{i}: float" for i in range(n)]
        f.write(f"energy fn model({', '.join(params)}) -> float {{\n")
        f.write("    let E = 0.0;\n")
        for i in range(n):
            if abs(h[i]) > 1e-9:
                f.write(f"    E = E + {h[i]:.6f} * s{i};\n")
            for j in range(i+1, n):
                if abs(J[i, j]) > 1e-9:
                    f.write(f"    E = E + {J[i, j]:.6f} * s{i} * s{j};\n")
        f.write("    return -1.0 * E;\n")
        f.write("}\n")
        f.write("fn main() -> void {\n")
        f.write("    let res = thermal_anneal(model, 6.0, 0.985, 4000);\n")
        f.write("}\n")

if __name__ == "__main__":
    generate_regular_varmix()
