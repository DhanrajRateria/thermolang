# Noise Shaping Research Harness

This folder is self-contained for ablations on variable-temperature sampling.

## What this provides
- Synthetic Ising benchmarks (dense, sparse, ring) with configurable sizes.
- Modes: global (off), degree, variance, degree+variance (matches compiler pass logic).
- Metrics: energy trajectory, acceptance rates, wall clock, beta min/max.
- Outputs: JSON and CSV (always), PNG plots if `matplotlib` is installed.

## Quick start
```bash
cd research/noise_shaping
python run_ablation.py --graphs dense sparse ring --nodes 40 --steps 200
```
Outputs land in `research/noise_shaping/outputs/`.

## Modes and knobs
- `--modes off degree variance degree+variance` (defaults to all)
- `--variance-shrink 0.5` sets the max beta shrink from variance (same knob as `NOISE_SHAPING_VARIANCE_SHRINK`).
- Set `--nodes`, `--steps`, and `--seed` for scale and reproducibility.

## Relating to compiler toggles
NoiseShapingPass reads environment variables:
- `NOISE_SHAPING_MODE`: `off`, `degree`, `variance`, `degree+variance` (default).
- `NOISE_SHAPING_VARIANCE_SHRINK`: float in [0,1], default 0.5.

Use these to run compiler-level baselines (global vs degree vs variance vs both) when generating code.

## Plotting
If `matplotlib` is available, PNGs are written alongside JSON/CSV. Otherwise, only JSON/CSV are produced.
