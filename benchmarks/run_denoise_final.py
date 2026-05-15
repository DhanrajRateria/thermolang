from __future__ import annotations

import argparse
import csv
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
from generate_denoise import generate_denoise_thermo, save_image


def denoise_energy(state: np.ndarray, noisy: np.ndarray, *, J: float, h: float) -> float:
    size = state.shape[0]
    e = 0.0

    for r in range(size):
        for c in range(size):
            if c + 1 < size:
                e += -J * state[r, c] * state[r, c + 1]

            if r + 1 < size:
                e += -J * state[r, c] * state[r + 1, c]

            e += -h * noisy[r, c] * state[r, c]

    return float(e)


def save_comparison(
    clean: np.ndarray,
    noisy: np.ndarray,
    recovered: np.ndarray,
    path: Path,
) -> None:
    fig, ax = plt.subplots(1, 3, figsize=(12, 4))

    for axis, arr, title in zip(
        ax,
        [clean, noisy, recovered],
        ["Clean", "Noisy", "Denoised"],
    ):
        axis.imshow(arr, cmap="gray", vmin=-1, vmax=1)
        axis.set_title(title)
        axis.axis("off")

    plt.tight_layout()
    plt.savefig(path, dpi=240)
    plt.close()


def run_single(
    seed: int,
    *,
    size: int,
    noise_prob: float,
    J: float,
    h: float,
    steps: int,
    out_root: Path,
) -> dict:
    name = f"denoise_{size}x{size}_seed{seed}"

    source = ROOT / "examples" / f"{name}.thermo"
    prefix = source.with_suffix("")
    run_dir = out_root / name
    run_dir.mkdir(parents=True, exist_ok=True)

    generate_denoise_thermo(
        source,
        size=size,
        seed=seed,
        noise_prob=noise_prob,
        J=J,
        h=h,
        steps=steps,
    )

    py_file, compile_log = compile_thermo(
        source,
        target="sim",
        no_opts=True,
        timeout=300,
    )

    clean = np.load(str(prefix) + "_target.npy")
    noisy = np.load(str(prefix) + "_noisy.npy")

    patch_generated_sim(
        py_file,
        seed=seed,
        initial_state=noisy.reshape(-1).tolist(),
    )

    sim = run_cmd([PYTHON, str(py_file)], timeout=300)
    sim_log = sim.stdout + sim.stderr

    if sim.returncode != 0:
        raise RuntimeError(f"Simulation failed for {name}\n{sim_log}")

    final = np.array(parse_final_state(sim_log), dtype=int).reshape((size, size))
    reported_energy = parse_reported_energy(sim_log)

    before = int(np.sum(clean != noisy))
    after = int(np.sum(clean != final))
    changed = int(np.sum(noisy != final))

    total = size * size

    acc_before = 100.0 * (total - before) / total
    acc_after = 100.0 * (total - after) / total
    improvement = 100.0 * (before - after) / before if before else 0.0

    e_noisy = denoise_energy(noisy, noisy, J=J, h=h)
    e_final = denoise_energy(final, noisy, J=J, h=h)
    e_clean = denoise_energy(clean, noisy, J=J, h=h)

    save_image(clean, run_dir / "clean.png", "Clean")
    save_image(noisy, run_dir / "noisy.png", "Noisy")
    save_image(final, run_dir / "denoised.png", "Denoised")
    save_comparison(clean, noisy, final, run_dir / "comparison.png")

    (run_dir / "compile_log.txt").write_text(compile_log, encoding="utf-8")
    (run_dir / "run_log.txt").write_text(sim_log, encoding="utf-8")

    shutil.copy2(source, run_dir / source.name)

    metrics = {
        "run": name,
        "seed": seed,
        "size": size,
        "noise_prob": noise_prob,
        "J": J,
        "h": h,
        "steps": steps,
        "hamming_before": before,
        "hamming_after": after,
        "pixels_changed_by_sampler": changed,
        "accuracy_before_pct": round(acc_before, 4),
        "accuracy_after_pct": round(acc_after, 4),
        "hamming_improvement_pct": round(improvement, 4),
        "energy_noisy": round(e_noisy, 6),
        "energy_final": round(e_final, 6),
        "energy_clean_reference": round(e_clean, 6),
        "compiler_reported_energy": reported_energy,
    }

    summary = "\n".join(f"{k}: {v}" for k, v in metrics.items()) + "\n"
    (run_dir / "denoising_metrics.txt").write_text(summary, encoding="utf-8")

    return metrics


def main() -> None:
    parser = argparse.ArgumentParser()

    parser.add_argument("--seeds", nargs="+", type=int, default=[11, 42, 77])
    parser.add_argument("--size", type=int, default=10)
    parser.add_argument("--noise-prob", type=float, default=0.20)
    parser.add_argument("--J", type=float, default=1.0)
    parser.add_argument("--h", type=float, default=2.5)
    parser.add_argument("--steps", type=int, default=2000)
    parser.add_argument("--out-dir", default="results/final_validation/01_denoising")

    args = parser.parse_args()

    out_root = ROOT / args.out_dir
    out_root.mkdir(parents=True, exist_ok=True)

    rows = [
        run_single(
            seed,
            size=args.size,
            noise_prob=args.noise_prob,
            J=args.J,
            h=args.h,
            steps=args.steps,
            out_root=out_root,
        )
        for seed in args.seeds
    ]

    csv_path = out_root / "aggregate_metrics.csv"

    with csv_path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)

    avg_before = np.mean([r["hamming_before"] for r in rows])
    avg_after = np.mean([r["hamming_after"] for r in rows])
    avg_acc = np.mean([r["accuracy_after_pct"] for r in rows])
    avg_imp = np.mean([r["hamming_improvement_pct"] for r in rows])

    aggregate = (
        "ThermoLang final validation: image denoising\n"
        f"runs: {len(rows)}\n"
        f"average_hamming_before: {avg_before:.3f}\n"
        f"average_hamming_after: {avg_after:.3f}\n"
        f"average_accuracy_after_pct: {avg_acc:.3f}\n"
        f"average_hamming_improvement_pct: {avg_imp:.3f}\n"
        "claim: noisy binary image restoration is expressed as an energy minimization problem; "
        "the sampler searches for a lower-energy clean state.\n"
    )

    (out_root / "aggregate_summary.txt").write_text(aggregate, encoding="utf-8")

    print(aggregate)
    print(f"Saved denoising validation artifacts in {out_root}")


if __name__ == "__main__":
    main()