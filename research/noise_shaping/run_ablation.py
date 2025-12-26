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
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple

import numpy as np


@dataclass
class RunResult:
    mode: str
    beta_min: float
    beta_max: float
    wall_clock_sec: float
    final_energy: float
    energies: List[float]
    acceptance: List[float]

    def to_dict(self) -> Dict:
        return {
            "mode": self.mode,
            "beta_min": self.beta_min,
            "beta_max": self.beta_max,
            "wall_clock_sec": self.wall_clock_sec,
            "final_energy": self.final_energy,
            "energies": self.energies,
            "acceptance": self.acceptance,
        }


def generate_graph(kind: str, n: int, seed: int) -> Tuple[np.ndarray, np.ndarray]:
    rng = np.random.default_rng(seed)
    if kind == "dense":
        p = 0.5
        mask = rng.random((n, n)) < p
    elif kind == "sparse":
        p = 0.1
        mask = rng.random((n, n)) < p
    elif kind == "ring":
        mask = np.zeros((n, n), dtype=bool)
        for i in range(n):
            mask[i, (i + 1) % n] = True
            mask[i, (i - 1) % n] = True
    else:
        raise ValueError(f"unknown graph kind: {kind}")

    mask = np.triu(mask, 1)
    weights = np.where(mask, rng.normal(0, 1, size=(n, n)), 0.0)
    J = weights + weights.T
    np.fill_diagonal(J, 0.0)

    h = rng.normal(0, 0.5, size=n)
    return J, h


def estimate_variances(J: np.ndarray, h: np.ndarray) -> np.ndarray:
    row_energy = np.sum(np.abs(J), axis=1)
    variances = row_energy * 0.5 + np.abs(h)
    return variances


def degree_beta(J: np.ndarray) -> np.ndarray:
    adj = (np.abs(J) > 1e-9).astype(float)
    np.fill_diagonal(adj, 0.0)
    degrees = adj.sum(axis=1)
    max_deg = degrees.max() if degrees.size else 0.0
    if max_deg <= 0.0:
        return np.ones_like(degrees)
    beta = 1.0 + degrees / max_deg
    return np.clip(beta, 1.0, 2.0)


def variance_shrink(base_beta: np.ndarray, variances: np.ndarray, cap: float) -> np.ndarray:
    if variances.size == 0:
        return base_beta
    max_var = variances.max()
    if max_var <= 0.0:
        return base_beta
    shrink = 1.0 - cap * (variances / max_var)
    shrink = np.clip(shrink, 0.1, 1.0)
    return base_beta * shrink


def build_beta(J: np.ndarray, variances: np.ndarray, mode: str, variance_cap: float) -> np.ndarray:
    mode_l = mode.lower()
    beta = np.ones(J.shape[0])
    if mode_l == "off":
        return beta
    if mode_l in ("degree", "degree+variance"):
        beta = degree_beta(J)
    if mode_l in ("variance", "degree+variance"):
        beta = variance_shrink(beta, variances, variance_cap)
    return beta


def energy(J: np.ndarray, h: np.ndarray, spins: np.ndarray) -> float:
    coupling = 0.5 * spins @ J @ spins
    field = h @ spins
    return -float(coupling + field)


def metropolis(J: np.ndarray, h: np.ndarray, beta: np.ndarray, steps: int, rng: np.random.Generator) -> Tuple[List[float], List[float]]:
    n = J.shape[0]
    spins = rng.choice([-1, 1], size=n)
    energies: List[float] = []
    acceptance: List[float] = []

    beta_vec = beta if beta.shape else np.array([beta])

    for _ in range(steps):
        accepted = 0
        for _ in range(n):
            i = rng.integers(0, n)
            local_field = h[i] + np.dot(J[i, :], spins)
            dE = 2.0 * spins[i] * local_field
            b = beta_vec[i] if beta_vec.size > 1 else beta_vec[0]
            if dE <= 0 or rng.random() < math.exp(-b * dE):
                spins[i] *= -1
                accepted += 1
        energies.append(energy(J, h, spins))
        acceptance.append(accepted / n)
    return energies, acceptance


def run_modes(J: np.ndarray, h: np.ndarray, modes: List[str], steps: int, variance_cap: float, seed: int) -> List[RunResult]:
    variances = estimate_variances(J, h)
    rng = np.random.default_rng(seed)
    results: List[RunResult] = []

    for mode in modes:
        beta = build_beta(J, variances, mode, variance_cap)
        t0 = time.perf_counter()
        energies, acc = metropolis(J, h, beta, steps, rng)
        t1 = time.perf_counter()
        results.append(
            RunResult(
                mode=mode,
                beta_min=float(beta.min()) if beta.size else float(beta),
                beta_max=float(beta.max()) if beta.size else float(beta),
                wall_clock_sec=t1 - t0,
                final_energy=float(energies[-1]),
                energies=energies,
                acceptance=acc,
            )
        )
    return results


def write_outputs(out_dir: Path, label: str, results: List[RunResult]) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    data = {r.mode: r.to_dict() for r in results}
    with open(out_dir / f"{label}.json", "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)

    # Flat CSV for quick plots
    rows: List[str] = []
    for r in results:
        rows.append(
            f"{r.mode},{r.beta_min:.4f},{r.beta_max:.4f},{r.wall_clock_sec:.6f},{r.final_energy:.6f}"
        )
    header = "mode,beta_min,beta_max,wall_clock_sec,final_energy"
    with open(out_dir / f"{label}.csv", "w", encoding="utf-8") as f:
        f.write(header + "\n")
        f.write("\n".join(rows))


def maybe_plot(out_dir: Path, label: str, results: List[RunResult]) -> None:
    try:
        import matplotlib.pyplot as plt  # type: ignore
    except Exception:
        return

    fig, ax = plt.subplots(2, 1, figsize=(7, 6), sharex=True)
    for r in results:
        ax[0].plot(r.energies, label=r.mode)
        ax[1].plot(r.acceptance, label=r.mode)
    ax[0].set_ylabel("Energy")
    ax[1].set_ylabel("Acceptance")
    ax[1].set_xlabel("Step")
    ax[0].legend()
    fig.tight_layout()
    out_dir.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_dir / f"{label}.png", dpi=150)
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser(description="Noise shaping ablation harness")
    parser.add_argument("--graphs", nargs="*", default=["dense", "sparse", "ring"], help="Graph types to run")
    parser.add_argument("--nodes", type=int, default=40, help="Number of spins")
    parser.add_argument("--steps", type=int, default=200, help="Sampler sweeps")
    parser.add_argument("--variance-shrink", type=float, default=0.5, help="Max beta shrink from variance")
    parser.add_argument("--modes", nargs="*", default=["off", "degree", "variance", "degree+variance"], help="Modes to run")
    parser.add_argument("--seed", type=int, default=7, help="Random seed")
    parser.add_argument("--label", type=str, default=None, help="Optional label for output files")
    args = parser.parse_args()

    out_dir = Path(__file__).parent / "outputs"
    label_base = args.label or f"n{args.nodes}_s{args.steps}_cap{args.variance_shrink}"

    for g in args.graphs:
        J, h = generate_graph(g, args.nodes, args.seed)
        results = run_modes(J, h, args.modes, args.steps, args.variance_shrink, args.seed + 1)
        label = f"{g}_{label_base}"
        write_outputs(out_dir, label, results)
        maybe_plot(out_dir, label, results)
        print(f"[run] graph={g}, outputs={out_dir / (label + '.json')}")


if __name__ == "__main__":
    main()
