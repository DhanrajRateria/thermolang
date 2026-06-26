from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np

from generated_artifacts import generated_artifact_path


def make_connected_random_graph(
    n: int,
    edge_prob: float,
    seed: int,
) -> list[tuple[int, int]]:
    rng = np.random.default_rng(seed)
    edges: set[tuple[int, int]] = set()

    # Start with a ring so the graph is connected and visually understandable.
    for i in range(n):
        j = (i + 1) % n
        edges.add((i, j) if i < j else (j, i))

    for i in range(n):
        for j in range(i + 1, n):
            if (i, j) not in edges and rng.random() < edge_prob:
                edges.add((i, j))

    return sorted(edges)


def generate_maxcut_thermo(
    filename: str | Path = "examples/maxcut_8node.thermo",
    *,
    n: int = 8,
    edge_prob: float = 0.45,
    seed: int = 7,
    steps: int = 3000,
) -> None:
    filename = Path(filename)
    filename.parent.mkdir(parents=True, exist_ok=True)

    edges = make_connected_random_graph(n, edge_prob, seed)

    with filename.open("w", encoding="utf-8") as f:
        f.write(
            f"// Max-Cut validation problem: n={n}, edges={len(edges)}, seed={seed}\n"
        )
        f.write(
            "// Minimize sum(s_i*s_j). Crossing edges contribute -1, "
            "same-side edges contribute +1.\n\n"
        )

        params = [f"s{i}: float" for i in range(n)]

        f.write(f"energy fn maxcut_model({', '.join(params)}) -> float {{\n")
        f.write("    let E = 0.0;\n")

        for i, j in edges:
            f.write(f"    E = E + 1.0 * s{i} * s{j};\n")

        f.write("    return E;\n")
        f.write("}\n\n")
        f.write("fn main() -> void {\n")
        f.write(f"    let result = thermal_anneal(maxcut_model, 5.0, 0.95, {steps});\n")
        f.write("}\n")

    metadata = {
        "n": n,
        "edge_prob": edge_prob,
        "seed": seed,
        "steps": steps,
        "edges": edges,
    }

    generated_artifact_path(filename, ".json").write_text(
        json.dumps(metadata, indent=2),
        encoding="utf-8",
    )

    print(f"Wrote {filename} with {n} nodes and {len(edges)} edges")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--filename", default="examples/maxcut_8node.thermo")
    parser.add_argument("--nodes", type=int, default=8)
    parser.add_argument("--edge-prob", type=float, default=0.45)
    parser.add_argument("--seed", type=int, default=7)
    parser.add_argument("--steps", type=int, default=3000)

    args = parser.parse_args()

    generate_maxcut_thermo(
        args.filename,
        n=args.nodes,
        edge_prob=args.edge_prob,
        seed=args.seed,
        steps=args.steps,
    )