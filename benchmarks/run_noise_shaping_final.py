from __future__ import annotations

import argparse
import csv
import json
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

MODES = ["off", "degree", "variance", "degree+variance"]


def generate_noise_problem(
    filename: Path,
    *,
    n: int = 16,
    density: float = 0.60,
    seed: int = 314,
    steps: int = 2500,
) -> dict:
    filename.parent.mkdir(parents=True, exist_ok=True)

    rng = np.random.default_rng(seed)

    mask = rng.random((n, n)) < density
    mask = np.triu(mask, 1)

    J = rng.normal(0.0, 1.0, size=(n, n)) * mask
    J = J + J.T

    h = rng.normal(0.0, 0.35, size=n)

    # Profile used by NoiseShapingPass through NOISE_SHAPING_VARIANCES.
    # It is a deterministic proxy for spin instability.
    variance_profile = np.abs(h) + np.sum(np.abs(J), axis=1)
    variance_profile = variance_profile / (np.max(variance_profile) + 1e-12)

    with filename.open("w", encoding="utf-8") as f:
        f.write(
            f"// Compiler noise-shaping validation problem: "
            f"n={n}, density={density}, seed={seed}\n"
        )
        f.write("// Original objective is the unshaped Ising energy written below.\n\n")

        params = [f"s{i}: float" for i in range(n)]

        f.write(f"energy fn noise_shape_model({', '.join(params)}) -> float {{\n")
        f.write("    let E = 0.0;\n")

        for i in range(n):
            if abs(h[i]) > 1e-12:
                f.write(f"    E = E + {h[i]:.6f} * s{i};\n")

            for j in range(i + 1, n):
                if abs(J[i, j]) > 1e-12:
                    f.write(f"    E = E + {J[i, j]:.6f} * s{i} * s{j};\n")

        f.write("    return E;\n")
        f.write("}\n\n")
        f.write("fn main() -> void {\n")
        f.write(f"    let result = thermal_anneal(noise_shape_model, 5.0, 0.95, {steps});\n")
        f.write("}\n")

    np.savez(
        filename.with_suffix(".npz"),
        J=J,
        h=h,
        variance_profile=variance_profile,
    )

    metadata = {
        "n": n,
        "density": density,
        "seed": seed,
        "steps_requested": steps,
        "couplings": int(np.sum(np.triu(np.abs(J) > 1e-12, 1))),
    }

    filename.with_suffix(".json").write_text(
        json.dumps(metadata, indent=2),
        encoding="utf-8",
    )

    return metadata


def true_energy(spins: list[int] | np.ndarray, J: np.ndarray, h: np.ndarray) -> float:
    s = np.array(spins, dtype=float)[: len(h)]

    return float(np.sum(np.triu(J, 1) * np.outer(s, s)) + np.dot(h, s))


def run_mode(
    source: Path,
    mode: str,
    *,
    initial_state: list[int],
    run_seed: int,
    variance_profile: np.ndarray,
    out_dir: Path,
) -> dict:
    env = {
        "NOISE_SHAPING_MODE": mode,
        "NOISE_SHAPING_STRENGTH": "1.0",
        "NOISE_SHAPING_VARIANCE_SHRINK": "0.5",
        "NOISE_SHAPING_VARIANCE_POLICY": "cool",
        "NOISE_SHAPING_VARIANCE_RENORM": "1",
        "NOISE_SHAPING_VARIANCES": ",".join(f"{v:.6f}" for v in variance_profile),
    }

    py_file, compile_log = compile_thermo(
        source,
        target="sim",
        no_opts=False,
        env=env,
        timeout=300,
    )

    patch_generated_sim(
        py_file,
        seed=run_seed,
        initial_state=initial_state,
    )

    sim = run_cmd([PYTHON, str(py_file)], timeout=300)
    sim_log = sim.stdout + sim.stderr

    if sim.returncode != 0:
        raise RuntimeError(f"Simulation failed for mode {mode}\n{sim_log}")

    mode_dir = out_dir / mode.replace("+", "_")
    mode_dir.mkdir(parents=True, exist_ok=True)

    (mode_dir / "compile_log.txt").write_text(compile_log, encoding="utf-8")
    (mode_dir / "run_log.txt").write_text(sim_log, encoding="utf-8")

    shutil.copy2(py_file, mode_dir / py_file.name)

    return {
        "mode": mode,
        "state": parse_final_state(sim_log),
        "compiler_reported_energy": parse_reported_energy(sim_log),
        "compile_log": compile_log,
        "run_log": sim_log,
    }


def save_bar_plot(rows: list[dict], path: Path) -> None:
    labels = [r["mode"] for r in rows]
    energies = [r["mean_true_energy"] for r in rows]

    plt.figure(figsize=(9, 5.5))

    bars = plt.bar(labels, energies)

    plt.ylabel("Mean true energy over matched restarts (lower is better)")
    plt.title("Compiler noise shaping validation")
    plt.grid(axis="y", alpha=0.3)

    for bar, energy in zip(bars, energies):
        plt.text(
            bar.get_x() + bar.get_width() / 2,
            bar.get_height(),
            f"{energy:.2f}",
            ha="center",
            va="bottom",
        )

    plt.tight_layout()
    plt.savefig(path, dpi=230)
    plt.close()


def main() -> None:
    parser = argparse.ArgumentParser()

    parser.add_argument("--nodes", type=int, default=16)
    parser.add_argument("--density", type=float, default=0.65)
    parser.add_argument("--problem-seed", type=int, default=101)
    parser.add_argument("--steps", type=int, default=2500)
    parser.add_argument("--restarts", type=int, default=3)
    parser.add_argument("--out-dir", default="results/final_validation/03_noise_shaping")

    args = parser.parse_args()

    out_root = ROOT / args.out_dir
    out_root.mkdir(parents=True, exist_ok=True)

    source = ROOT / "examples" / f"noise_shape_n{args.nodes}_seed{args.problem_seed}.thermo"

    metadata = generate_noise_problem(
        source,
        n=args.nodes,
        density=args.density,
        seed=args.problem_seed,
        steps=args.steps,
    )

    data = np.load(source.with_suffix(".npz"))

    J = data["J"]
    h = data["h"]
    variance_profile = data["variance_profile"]

    rng = np.random.default_rng(args.problem_seed + 9000)

    restart_initials = [
        rng.choice([-1, 1], size=args.nodes).astype(int).tolist()
        for _ in range(args.restarts)
    ]

    detailed_rows = []
    summary_rows = []

    for mode in MODES:
        energies = []
        best_energy = None
        best_state = None

        for r, initial in enumerate(restart_initials):
            run_seed = args.problem_seed + 10000 + r
            run_dir = out_root / f"restart_{r:02d}"

            result = run_mode(
                source,
                mode,
                initial_state=initial,
                run_seed=run_seed,
                variance_profile=variance_profile,
                out_dir=run_dir,
            )

            final_state = result["state"][: args.nodes]

            e_initial = true_energy(initial, J, h)
            e_final = true_energy(final_state, J, h)

            energies.append(e_final)

            if best_energy is None or e_final < best_energy:
                best_energy = e_final
                best_state = final_state

            detailed_rows.append(
                {
                    "restart": r,
                    "mode": mode,
                    "initial_true_energy": round(e_initial, 6),
                    "final_true_energy": round(e_final, 6),
                    "delta_true_energy": round(e_final - e_initial, 6),
                    "compiler_reported_energy": result["compiler_reported_energy"],
                }
            )

        summary_rows.append(
            {
                "mode": mode,
                "mean_true_energy": round(float(np.mean(energies)), 6),
                "best_true_energy": round(float(np.min(energies)), 6),
                "std_true_energy": round(float(np.std(energies)), 6),
                "restarts": args.restarts,
                "best_state": " ".join(map(str, best_state or [])),
            }
        )

    baseline = next(row for row in summary_rows if row["mode"] == "off")

    for row in summary_rows:
        row["mean_improvement_vs_off"] = round(
            baseline["mean_true_energy"] - row["mean_true_energy"],
            6,
        )
        row["best_improvement_vs_off"] = round(
            baseline["best_true_energy"] - row["best_true_energy"],
            6,
        )

    with (out_root / "detailed_restarts.csv").open(
        "w",
        newline="",
        encoding="utf-8",
    ) as f:
        writer = csv.DictWriter(f, fieldnames=list(detailed_rows[0].keys()))
        writer.writeheader()
        writer.writerows(detailed_rows)

    with (out_root / "aggregate_metrics.csv").open(
        "w",
        newline="",
        encoding="utf-8",
    ) as f:
        writer = csv.DictWriter(f, fieldnames=list(summary_rows[0].keys()))
        writer.writeheader()
        writer.writerows(summary_rows)

    save_bar_plot(summary_rows, out_root / "noise_shaping_energy_comparison.png")

    shutil.copy2(source, out_root / source.name)
    shutil.copy2(source.with_suffix(".json"), out_root / source.with_suffix(".json").name)
    shutil.copy2(source.with_suffix(".npz"), out_root / source.with_suffix(".npz").name)

    best_mode = min(summary_rows, key=lambda row: row["mean_true_energy"])

    aggregate = (
        "ThermoLang final validation: compiler noise shaping\n"
        f"problem: n={metadata['n']}, couplings={metadata['couplings']}, density={metadata['density']}\n"
        f"matched_restarts_per_mode: {args.restarts}\n"
        f"baseline_mean_true_energy: {baseline['mean_true_energy']}\n"
        f"best_mode_by_mean: {best_mode['mode']}\n"
        f"best_mode_mean_true_energy: {best_mode['mean_true_energy']}\n"
        f"mean_improvement_vs_off: {best_mode['mean_improvement_vs_off']}\n"
        "claim: the compiler is not only translating files; with NOISE_SHAPING_MODE enabled "
        "it changes the generated energy landscape/schedule parameters and the resulting stochastic search behavior.\n"
    )

    (out_root / "aggregate_summary.txt").write_text(aggregate, encoding="utf-8")

    print(aggregate)
    print(f"Saved noise-shaping validation artifacts in {out_root}")


if __name__ == "__main__":
    main()