from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


def save_image(arr: np.ndarray, path: Path, title: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)

    plt.figure(figsize=(4, 4))
    plt.imshow(arr, cmap="gray", vmin=-1, vmax=1)
    plt.title(title)
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(path, dpi=220)
    plt.close()


def make_clean_plus(size: int) -> np.ndarray:
    clean = -1 * np.ones((size, size), dtype=int)

    mid = size // 2
    clean[mid, :] = 1
    clean[:, mid] = 1

    return clean


def generate_denoise_thermo(
    filename: str | Path = "examples/denoise_10x10.thermo",
    *,
    size: int = 10,
    seed: int = 42,
    noise_prob: float = 0.20,
    J: float = 1.0,
    h: float = 2.5,
    steps: int = 2000,
) -> None:
    filename = Path(filename)
    filename.parent.mkdir(parents=True, exist_ok=True)

    prefix = filename.with_suffix("")

    print(
        f"Generating denoising problem: size={size}x{size}, seed={seed}, "
        f"noise_prob={noise_prob}, J={J}, h={h}, steps={steps}"
    )

    clean = make_clean_plus(size)

    rng = np.random.default_rng(seed)
    noise_mask = rng.random((size, size)) < noise_prob

    noisy = clean.copy()
    noisy[noise_mask] *= -1

    with filename.open("w", encoding="utf-8") as f:
        f.write(f"// Image Denoising {size}x{size}\n")
        f.write(f"// seed={seed}, noise_prob={noise_prob}, J={J}, h={h}\n")
        f.write("// Energy: smoothing + noisy-observation data fidelity\n\n")

        params = [f"s{i}: float" for i in range(size * size)]

        f.write("energy fn denoise_model(\n    ")
        f.write(",\n    ".join(params))
        f.write("\n) -> float {\n")
        f.write("    let E = 0.0;\n")

        coupling_count = 0

        for r in range(size):
            for c in range(size):
                i = r * size + c

                if c + 1 < size:
                    j = r * size + c + 1
                    f.write(f"    E = E + {-J:.6f} * s{i} * s{j};\n")
                    coupling_count += 1

                if r + 1 < size:
                    j = (r + 1) * size + c
                    f.write(f"    E = E + {-J:.6f} * s{i} * s{j};\n")
                    coupling_count += 1

        f.write(f"\n    // {coupling_count} nearest-neighbor smoothing couplings.\n")

        for i in range(size * size):
            r, c = divmod(i, size)
            bias = -h * int(noisy[r, c])
            f.write(f"    E = E + {bias:.6f} * s{i};\n")

        f.write("    return E;\n")
        f.write("}\n\n")
        f.write("fn main() -> void {\n")
        f.write(f"    let res = thermal_anneal(denoise_model, 5.0, 0.95, {steps});\n")
        f.write("}\n")

    np.save(str(prefix) + "_target.npy", clean)
    np.save(str(prefix) + "_noisy.npy", noisy)
    np.save(str(prefix) + "_noise_mask.npy", noise_mask.astype(np.uint8))

    save_image(clean, Path(str(prefix) + "_clean.png"), "Clean")
    save_image(noisy, Path(str(prefix) + "_noisy.png"), "Noisy")

    print(f"Wrote {filename}")
    print(f"Noisy flips: {int(np.sum(clean != noisy))}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument("--filename", default="examples/denoise_10x10.thermo")
    parser.add_argument("--size", type=int, default=10)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--noise-prob", type=float, default=0.20)
    parser.add_argument("--J", type=float, default=1.0)
    parser.add_argument("--h", type=float, default=2.5)
    parser.add_argument("--steps", type=int, default=2000)

    args = parser.parse_args()

    generate_denoise_thermo(
        args.filename,
        size=args.size,
        seed=args.seed,
        noise_prob=args.noise_prob,
        J=args.J,
        h=args.h,
        steps=args.steps,
    )