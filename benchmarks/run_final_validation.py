from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run_step(label: str, args: list[str]) -> None:
    print("=" * 90)
    print(label)
    print("=" * 90)

    res = subprocess.run(
        [sys.executable, *args],
        cwd=str(ROOT),
        text=True,
    )

    if res.returncode != 0:
        raise SystemExit(f"{label} failed with exit code {res.returncode}")


def main() -> None:
    run_step("01 / Image denoising", ["benchmarks/run_denoise_final.py"])
    run_step("02 / Max-Cut graph optimization", ["benchmarks/run_maxcut_final.py"])
    run_step("03 / Compiler noise shaping", ["benchmarks/run_noise_shaping_final.py"])

    print("All final validation experiments completed.")
    print("Artifacts saved under results/final_validation/")


if __name__ == "__main__":
    main()