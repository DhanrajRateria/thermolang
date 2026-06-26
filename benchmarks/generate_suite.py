# benchmarks/generate_suite.py
import os
import numpy as np

from generated_artifacts import generated_artifact_path

def ensure_dir(path: str):
    os.makedirs(os.path.dirname(path), exist_ok=True)

def save_npz(base_thermo_path: str, J: np.ndarray, h: np.ndarray):
    np.savez(generated_artifact_path(base_thermo_path, ".npz"), J=J, h=h)

def symmetrize(J: np.ndarray) -> np.ndarray:
    J = np.triu(J, 1)
    return J + J.T

def write_thermo_from_Jh(
    path: str,
    fn_name: str,
    J: np.ndarray,
    h: np.ndarray,
    T0: float,
    alpha: float,
    steps: int,
    header_comment: str = ""
):
    """
    CANONICAL convention (matches calc_true_energy in run_suite.py):

      Spins: s_i ∈ {-1, +1}

      True Ising energy:
        E(s) = -0.5 * s^T J s - h^T s
             = - sum_{i<j} J_ij s_i s_j - sum_i h_i s_i

    We therefore emit ThermoLang energy fn as:
        E = Σ (-h_i) s_i + Σ_{i<j} (-J_ij) s_i s_j
        return E
    """
    ensure_dir(path)
    n = len(h)
    with open(path, "w") as f:
        if header_comment:
            f.write(f"// {header_comment}\n")
        f.write(f"// Auto-generated benchmark: {fn_name}, N={n}\n\n")
        params = [f"s{i}: float" for i in range(n)]
        f.write(f"energy fn {fn_name}({', '.join(params)}) -> float {{\n")
        f.write("    let E = 0.0;\n")

        # Linear terms: -h_i * s_i
        for i in range(n):
            hi = float(h[i])
            if abs(hi) > 1e-12:
                f.write(f"    E = E + {-hi:.6f} * s{i};\n")

        # Pairwise terms: -J_ij * s_i * s_j (only i<j)
        for i in range(n):
            for j in range(i + 1, n):
                Jij = float(J[i, j])
                if abs(Jij) > 1e-12:
                    f.write(f"    E = E + {-Jij:.6f} * s{i} * s{j};\n")

        f.write("    return E;\n")
        f.write("}\n\n")
        f.write("fn main() -> void {\n")
        f.write(f"    let res = thermal_anneal({fn_name}, {T0}, {alpha}, {steps});\n")
        f.write("}\n")


# -------------------
# Benchmark generators
# -------------------

def generate_dense(n=40, density=0.5, seed=42, out="examples/dense_40.thermo"):
    rng = np.random.default_rng(seed)
    mask = rng.random((n, n)) < density
    mask = np.triu(mask, 1)
    J = rng.normal(0, 1, (n, n)) * mask
    J = symmetrize(J)
    h = rng.normal(0, 0.5, n)

    save_npz(out, J, h)
    write_thermo_from_Jh(
        out, "model", J, h,
        T0=5.0, alpha=0.98, steps=2000,
        header_comment=f"Dense Random Graph (N={n}, density={density}, seed={seed})"
    )
    return out

def generate_sparse(n=40, p=0.08, seed=43, out="examples/sparse_40.thermo"):
    """
    Sparse random graph. Good 'control' where degree-shaping shouldn't dominate as strongly as dense,
    and where variance effects may appear depending on weight distribution.
    """
    rng = np.random.default_rng(seed)
    J = np.zeros((n, n), dtype=float)
    for i in range(n):
        for j in range(i + 1, n):
            if rng.random() < p:
                J[i, j] = rng.normal(0, 1.0)
    J = symmetrize(J)
    h = np.zeros(n, dtype=float)

    save_npz(out, J, h)
    write_thermo_from_Jh(
        out, "model", J, h,
        T0=5.0, alpha=0.98, steps=2500,
        header_comment=f"Sparse Random Graph (N={n}, p={p}, seed={seed})"
    )
    return out

def generate_ring(n=40, Jval=1.0, out="examples/ring_40.thermo"):
    """
    Ring graph (uniform degree=2). This is an important control:
    degree shaping should provide little/no improvement here.
    """
    J = np.zeros((n, n), dtype=float)
    h = np.zeros(n, dtype=float)
    for i in range(n):
        j = (i + 1) % n
        J[i, j] = Jval
        J[j, i] = Jval

    save_npz(out, J, h)
    write_thermo_from_Jh(
        out, "model", J, h,
        T0=5.0, alpha=0.98, steps=2000,
        header_comment=f"Ring Graph (N={n}, J={Jval})"
    )
    return out

def generate_star(n=50, seed=7, out="examples/star_50.thermo"):
    """
    Star with frustration chain among leaves (your idea), encoded in J.
    """
    rng = np.random.default_rng(seed)
    J = np.zeros((n, n), dtype=float)
    h = np.zeros(n, dtype=float)

    # center connections: ferromagnetic -1.0
    for i in range(1, n):
        J[0, i] = -1.0
        J[i, 0] = -1.0

    # leaf frustration: +0.5 between i and i+1
    for i in range(1, n - 1):
        J[i, i + 1] = 0.5
        J[i + 1, i] = 0.5

    save_npz(out, J, h)
    write_thermo_from_Jh(
        out, "star_model", J, h,
        T0=5.0, alpha=0.98, steps=2000,
        header_comment=f"Star Graph with Leaf Frustration (N={n}, seed={seed})"
    )
    return out

def generate_barbell(n_clique=10, Jval=-1.0, out="examples/barbell.thermo"):
    """
    TRUE barbell: two cliques connected by a single bridge EDGE.
    Total N = 2*n_clique.
    """
    n = 2 * n_clique
    J = np.zeros((n, n), dtype=float)
    h = np.zeros(n, dtype=float)

    # clique A: [0..n_clique-1]
    for i in range(n_clique):
        for j in range(i + 1, n_clique):
            J[i, j] = Jval

    # clique B: [n_clique..2*n_clique-1]
    for i in range(n_clique, n):
        for j in range(i + 1, n):
            J[i, j] = Jval

    # single bridge edge between last of A and first of B
    J[n_clique - 1, n_clique] = Jval

    J = symmetrize(J)

    save_npz(out, J, h)
    write_thermo_from_Jh(
        out, "barbell", J, h,
        T0=5.0, alpha=0.98, steps=5000,
        header_comment=f"Barbell Graph: two cliques (size={n_clique}) + single bridge edge"
    )
    return out

def generate_bridge_node_barbell(n_cluster=10, out="examples/barbell_bridge_node.thermo"):
    """
    OPTIONAL: your original 'bridge node' variant (not a classic barbell).
    Keep it because it can be a nice 'high variance centrality' demo later.
    Total N = 2*n_cluster + 1
    """
    total = n_cluster * 2 + 1
    bridge = n_cluster
    start_right = bridge + 1

    J = np.zeros((total, total), dtype=float)
    h = np.zeros(total, dtype=float)

    # left clique
    for i in range(n_cluster):
        for j in range(i + 1, n_cluster):
            J[i, j] = -1.0

    # right clique
    for i in range(start_right, total):
        for j in range(i + 1, total):
            J[i, j] = -1.0

    # bridge connects to one node in each clique
    J[0, bridge] = -1.0
    J[bridge, start_right] = -1.0

    J = symmetrize(J)

    save_npz(out, J, h)
    write_thermo_from_Jh(
        out, "barbell", J, h,
        T0=5.0, alpha=0.98, steps=5000,
        header_comment=f"Bridge-node Barbell Variant: N={total} (bridge index={bridge})"
    )
    return out

def generate_k5(out="examples/k5_graph.thermo"):
    """
    Fully connected K5 with antiferromagnetic (+1) couplings.
    """
    n = 5
    J = np.ones((n, n), dtype=float) - np.eye(n, dtype=float)  # +1 off-diagonal
    h = np.zeros(n, dtype=float)

    save_npz(out, J, h)
    write_thermo_from_Jh(
        out, "k5_model", J, h,
        T0=10.0, alpha=0.95, steps=1000,
        header_comment="K5 (non-planar) antiferromagnetic frustration; also useful for embedding paper"
    )
    return out

def generate_regular_varmix(n=40, k=6, seed=123, out="examples/regular_varmix_40.thermo"):
    """
    Uniform degree but bimodal coupling magnitudes to showcase variance shaping.
    IMPORTANT: build symmetric J directly (avoid overwrite->triu truncation artifacts).
    """
    rng = np.random.default_rng(seed)
    offsets = [1, 2, 5]  # degree = 6 via ± offsets
    assert 2 * len(offsets) == k

    J = np.zeros((n, n), dtype=float)
    strong, weak = 2.5, 0.3

    def set_edge(i, j, val):
        a, b = (i, j) if i < j else (j, i)
        J[a, b] = val

    for i in range(n):
        for off in offsets:
            for j in ((i + off) % n, (i - off) % n):
                mag = strong if rng.random() < 0.5 else weak
                sign = 1.0 if rng.random() < 0.5 else -1.0
                set_edge(i, j, sign * mag)

    J = symmetrize(J)
    h = rng.normal(0, 0.2, n)

    save_npz(out, J, h)
    write_thermo_from_Jh(
        out, "varmix_model", J, h,
        T0=6.0, alpha=0.985, steps=4000,
        header_comment=f"Regular VarMix (uniform degree k={k}, bimodal magnitudes), seed={seed}"
    )
    return out


def main():
    generated = []
    generated.append(generate_dense())
    generated.append(generate_sparse())
    generated.append(generate_ring())
    generated.append(generate_star())
    generated.append(generate_barbell())
    generated.append(generate_bridge_node_barbell())  # optional extra
    generated.append(generate_k5())
    generated.append(generate_regular_varmix())

    print("Generated benchmarks:")
    for p in generated:
        print("  -", p, "(+", p.replace(".thermo", ".npz"), ")")

if __name__ == "__main__":
    main()
