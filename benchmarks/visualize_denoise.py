import numpy as np
import matplotlib.pyplot as plt
import sys
import re
from pathlib import Path


def parse_final_state(sim_output_file, size):
    with open(sim_output_file, "r", encoding="utf-8") as f:
        content = f.read()

    match = re.search(r"\[FINAL_STATE\]:\s*\[(.*?)\]", content, re.DOTALL)
    if not match:
        return None

    vals = [int(x.strip()) for x in match.group(1).split(",") if x.strip()]
    if len(vals) != size * size:
        return None

    return np.array(vals, dtype=int).reshape((size, size))


def save_single(arr, path, title):
    plt.figure(figsize=(4, 4))
    plt.imshow(arr, cmap="gray", vmin=-1, vmax=1)
    plt.title(title)
    plt.axis("off")
    plt.tight_layout()
    plt.savefig(path, dpi=200)
    plt.close()


def parse_output_and_plot(sim_output_file, data_prefix, out_dir):
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    clean = np.load(data_prefix + "_target.npy")
    noisy = np.load(data_prefix + "_noisy.npy")
    size = clean.shape[0]

    recovered = parse_final_state(sim_output_file, size)
    if recovered is None:
        print("Error: Could not parse FINAL_STATE from simulation output.")
        return None

    save_single(clean, out_dir / "clean.png", "Clean")
    save_single(noisy, out_dir / "noisy.png", "Noisy")
    save_single(recovered, out_dir / "denoised.png", "Denoised")

    fig, ax = plt.subplots(1, 3, figsize=(12, 4))
    ax[0].imshow(clean, cmap="gray", vmin=-1, vmax=1)
    ax[0].set_title("Clean")
    ax[0].axis("off")

    ax[1].imshow(noisy, cmap="gray", vmin=-1, vmax=1)
    ax[1].set_title("Noisy")
    ax[1].axis("off")

    ax[2].imshow(recovered, cmap="gray", vmin=-1, vmax=1)
    ax[2].set_title("Denoised")
    ax[2].axis("off")

    plt.tight_layout()
    plt.savefig(out_dir / "comparison.png", dpi=220)
    plt.close()

    print(f"Saved denoising images in {out_dir}")
    return recovered


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python visualize_denoise.py <sim_output.txt> <data_file_prefix> <out_dir>")
    else:
        parse_output_and_plot(sys.argv[1], sys.argv[2], sys.argv[3])