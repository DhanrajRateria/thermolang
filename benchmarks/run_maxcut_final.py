from __future__ import annotations

import argparse
import csv
import itertools
import json
import math
import shutil
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np

from final_validation_common import (
    PYTHON,
    ROOT,
    compile_thermo,
    parse_final_state,
    parse_reported_energy,
    patch_generated_sim,
    run_cmd,
)
from generate_maxcut import generate_maxcut_thermo
from generated_artifacts import generated_artifact_path


def cut_value(spins: list[int] | np.ndarray, edges: list[tuple[int, int]]) -> int:
    s = np.array(spins, dtype=int)

    return int(sum(1 for i, j in edges if s[i] != s[j]))


def energy_value(spins: list[int] | np.ndarray, edges: list[tuple[int, int]]) -> float:
    s = np.array(spins, dtype=int)

    return float(sum(s[i] * s[j] for i, j in edges))


def brute_force_optimum(n: int, edges: list[tuple[int, int]]) -> tuple[int, list[int]]:
    best_cut = -1
    best_state: list[int] = []

    # Fix first spin to +1 to remove global sign symmetry.
    for bits in itertools.product([-1, 1], repeat=n - 1):
        state = [1, *bits]
        val = cut_value(state, edges)

        if val > best_cut:
            best_cut = val
            best_state = state

    return best_cut, best_state


def circular_positions(n: int) -> dict[int, tuple[float, float]]:
    return {
        i: (
            math.cos(2 * math.pi * i / n),
            math.sin(2 * math.pi * i / n),
        )
        for i in range(n)
    }


def draw_graph(
    n: int,
    edges: list[tuple[int, int]],
    spins: list[int],
    path: Path,
    title: str,
) -> None:
    pos = circular_positions(n)

    plt.figure(figsize=(5.5, 5.5))

    for i, j in edges:
        xi, yi = pos[i]
        xj, yj = pos[j]

        crossing = spins[i] != spins[j]

        plt.plot(
            [xi, xj],
            [yi, yj],
            linewidth=2.6 if crossing else 1.0,
            alpha=0.95 if crossing else 0.35,
        )

    for i in range(n):
        x, y = pos[i]
        marker = "o" if spins[i] == 1 else "s"

        plt.scatter(
            [x],
            [y],
            s=520,
            marker=marker,
            edgecolors="black",
            linewidths=1.2,
        )
        plt.text(
            x,
            y,
            str(i),
            ha="center",
            va="center",
            fontsize=10,
            fontweight="bold",
        )

    plt.title(title)
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(path, dpi=230)
    plt.close()


def save_comparison(
    n: int,
    edges: list[tuple[int, int]],
    initial: list[int],
    final: list[int],
    path: Path,
) -> None:
    pos = circular_positions(n)

    fig, ax = plt.subplots(1, 2, figsize=(11, 5.5))

    panels = [
        (ax[0], initial, f"Initial cut = {cut_value(initial, edges)}"),
        (ax[1], final, f"Optimized cut = {cut_value(final, edges)}"),
    ]

    for axis, spins, title in panels:
        for i, j in edges:
            xi, yi = pos[i]
            xj, yj = pos[j]

            crossing = spins[i] != spins[j]

            axis.plot(
                [xi, xj],
                [yi, yj],
                linewidth=2.6 if crossing else 1.0,
                alpha=0.95 if crossing else 0.35,
            )

        for i in range(n):
            x, y = pos[i]
            marker = "o" if spins[i] == 1 else "s"

            axis.scatter(
                [x],
                [y],
                s=520,
                marker=marker,
                edgecolors="black",
                linewidths=1.2,
            )
            axis.text(
                x,
                y,
                str(i),
                ha="center",
                va="center",
                fontsize=10,
                fontweight="bold",
            )

        axis.set_title(title)
        axis.axis("off")

    plt.tight_layout()
    plt.savefig(path, dpi=230)
    plt.close()


def run_single(
    seed: int,
    *,
    n: int,
    edge_prob: float,
    steps: int,
    out_root: Path,
) -> dict:
    name = f"maxcut_n{n}_seed{seed}"

    source = ROOT / "examples" / f"{name}.thermo"
    run_dir = out_root / name
    run_dir.mkdir(parents=True, exist_ok=True)

    generate_maxcut_thermo(
        source,
        n=n,
        edge_prob=edge_prob,
        seed=seed,
        steps=steps,
    )

    metadata = json.loads(generated_artifact_path(source, ".json").read_text(encoding="utf-8"))
    edges = [tuple(edge) for edge in metadata["edges"]]

    rng = np.random.default_rng(seed + 1000)
    initial = rng.choice([-1, 1], size=n).astype(int).tolist()

    py_file, compile_log = compile_thermo(
        source,
        target="sim",
        no_opts=True,
        timeout=300,
    )

    patch_generated_sim(
        py_file,
        seed=seed + 2000,
        initial_state=initial,
    )

    sim = run_cmd([PYTHON, str(py_file)], timeout=300)
    sim_log = sim.stdout + sim.stderr

    if sim.returncode != 0:
        raise RuntimeError(f"Simulation failed for {name}\n{sim_log}")

    final = parse_final_state(sim_log)[:n]
    reported_energy = parse_reported_energy(sim_log)

    initial_cut = cut_value(initial, edges)
    final_cut = cut_value(final, edges)

    optimum_cut, optimum_state = brute_force_optimum(n, edges)

    improvement = final_cut - initial_cut
    improvement_pct = 100.0 * improvement / initial_cut if initial_cut else 0.0
    optimality_gap = optimum_cut - final_cut

    draw_graph(
        n,
        edges,
        initial,
        run_dir / "initial_partition.png",
        f"Initial partition, cut={initial_cut}",
    )

    draw_graph(
        n,
        edges,
        final,
        run_dir / "final_partition.png",
        f"Final partition, cut={final_cut}",
    )

    save_comparison(
        n,
        edges,
        initial,
        final,
        run_dir / "comparison.png",
    )

    (run_dir / "compile_log.txt").write_text(compile_log, encoding="utf-8")
    (run_dir / "run_log.txt").write_text(sim_log, encoding="utf-8")

    shutil.copy2(source, run_dir / source.name)
    shutil.copy2(generated_artifact_path(source, ".json"), run_dir / source.with_suffix(".json").name)

    metrics = {
        "run": name,
        "seed": seed,
        "nodes": n,
        "edges": len(edges),
        "steps": steps,
        "initial_cut": initial_cut,
        "final_cut": final_cut,
        "best_possible_cut": optimum_cut,
        "optimality_gap": optimality_gap,
        "cut_improvement": improvement,
        "cut_improvement_pct": round(improvement_pct, 4),
        "initial_energy": energy_value(initial, edges),
        "final_energy": energy_value(final, edges),
        "compiler_reported_energy": reported_energy,
    }

    (run_dir / "maxcut_metrics.txt").write_text(
        "\n".join(f"{k}: {v}" for k, v in metrics.items()) + "\n",
        encoding="utf-8",
    )

    return metrics


def main() -> None:
    parser = argparse.ArgumentParser()

    parser.add_argument("--seeds", nargs="+", type=int, default=[7, 21, 42])
    parser.add_argument("--nodes", type=int, default=8)
    parser.add_argument("--edge-prob", type=float, default=0.45)
    parser.add_argument("--steps", type=int, default=3000)
    parser.add_argument("--out-dir", default="results/final_validation/02_maxcut")

    args = parser.parse_args()

    out_root = ROOT / args.out_dir
    out_root.mkdir(parents=True, exist_ok=True)

    rows = [
        run_single(
            seed,
            n=args.nodes,
            edge_prob=args.edge_prob,
            steps=args.steps,
            out_root=out_root,
        )
        for seed in args.seeds
    ]

    with (out_root / "aggregate_metrics.csv").open(
        "w",
        newline="",
        encoding="utf-8",
    ) as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    avg_initial = np.mean([r["initial_cut"] for r in rows])
    avg_final = np.mean([r["final_cut"] for r in rows])
    avg_gap = np.mean([r["optimality_gap"] for r in rows])

    aggregate = (
        "ThermoLang final validation: Max-Cut / graph optimization\n"
        f"runs: {len(rows)}\n"
        f"average_initial_cut: {avg_initial:.3f}\n"
        f"average_final_cut: {avg_final:.3f}\n"
        f"average_optimality_gap: {avg_gap:.3f}\n"
        "claim: Max-Cut is expressed as an Ising/QUBO-style energy; "
        "annealing searches for a partition with more crossing edges.\n"
    )

    (out_root / "aggregate_summary.txt").write_text(aggregate, encoding="utf-8")

    print(aggregate)
    print(f"Saved Max-Cut validation artifacts in {out_root}")


if __name__ == "__main__":
    main()