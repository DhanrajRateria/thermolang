# benchmarks/collect_variance.py
import os
import re
import subprocess
import argparse
import numpy as np

COMPILER = "./build/thermolangc"

def parse_variances(stdout: str):
    m = re.search(r"\[VARIANCES\]:\s*(.*)", stdout)
    if not m:
        return None
    s = m.group(1).strip()
    if s == "error":
        return None
    parts = [p.strip() for p in s.split(",") if p.strip() != ""]
    try:
        return [float(x) for x in parts]
    except:
        return None

def parse_seed_check(stdout: str):
    m = re.search(r"\[SEED_CHECK\]:\s*\[(.*?)\]", stdout)
    return m.group(1).strip() if m else None

def patch_seed(py_file: str, seed: int):
    subprocess.run(["python3", "benchmarks/patch_seed.py", py_file, str(seed)], check=True)

def compile_to_thrml(thermo_file: str, mode: str, strength: float = 1.0, extra_env=None):
    env = os.environ.copy()
    env["NOISE_SHAPING_MODE"] = mode
    env["NOISE_SHAPING_VARIANCE_SHRINK"] = env.get("NOISE_SHAPING_VARIANCE_SHRINK", "0.5")
    env["NOISE_SHAPING_STRENGTH"] = str(strength)
    if extra_env:
        env.update(extra_env)

    subprocess.run(
        [COMPILER, thermo_file, "--target=thrml"],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=True
    )

def run_thrml(py_file: str, extra_env=None):
    env = os.environ.copy()
    if extra_env:
        env.update(extra_env)
    res = subprocess.run(["python3", py_file], capture_output=True, text=True, env=env)
    return res.stdout

def ensure_dir(path: str):
    os.makedirs(path, exist_ok=True)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--thermo", required=True, help="Path to .thermo file (e.g., examples/barbell.thermo)")
    ap.add_argument("--seeds", type=int, default=10, help="Number of seeds to profile")
    ap.add_argument("--mode", default="off", choices=["off", "degree"], help="Profile dynamics under this shaping mode")
    ap.add_argument("--strength", type=float, default=1.0, help="Strength lambda used during profiling (usually 1.0)")
    ap.add_argument("--outdir", default="results/variances", help="Output directory")
    ap.add_argument("--aggregate", default="mean", choices=["mean", "median"], help="Aggregate variances across seeds")
    args = ap.parse_args()

    thermo_file = args.thermo
    base = os.path.splitext(os.path.basename(thermo_file))[0]
    py_file = f"{base}_thrml.py"

    ensure_dir(args.outdir)

    per_seed = []
    ok_seeds = []

    for seed in range(args.seeds):
        # Compile once per seed (safe, consistent with your pipeline)
        compile_to_thrml(thermo_file, args.mode, strength=args.strength)
        patch_seed(py_file, seed)

        out = run_thrml(py_file, extra_env={"THERMOLANG_PROFILE_VARIANCE": "1"})
        v = parse_variances(out)
        sc = parse_seed_check(out)

        if v is None:
            print(f"[WARN] {base}: seed={seed} variance parse failed. seed_check=[{sc}]")
            continue

        per_seed.append(v)
        ok_seeds.append(seed)
        print(f"[OK]   {base}: seed={seed} collected var_len={len(v)} seed_check=[{sc}]")

    if len(per_seed) == 0:
        raise RuntimeError(f"No variance profiles collected for {base}. Check runtime prints [VARIANCES].")

    # Make all rows same length (trim to min length)
    min_len = min(len(x) for x in per_seed)
    arr = np.array([x[:min_len] for x in per_seed], dtype=np.float64)

    if args.aggregate == "mean":
        agg = np.mean(arr, axis=0)
    else:
        agg = np.median(arr, axis=0)

    # Save:
    # 1) aggregate file: results/variances/{problem}.csv   (single line)
    agg_path = os.path.join(args.outdir, f"{base}.csv")
    with open(agg_path, "w") as f:
        f.write(",".join(f"{x:.6f}" for x in agg.tolist()) + "\n")

    # 2) optional diagnostics: results/variances/{problem}_per_seed.npz
    npz_path = os.path.join(args.outdir, f"{base}_per_seed.npz")
    np.savez(npz_path, variances=arr, seeds=np.array(ok_seeds, dtype=np.int32))

    print(f"\nSaved aggregate variances: {agg_path}")
    print(f"Saved per-seed variances:  {npz_path}")
    print(f"Used seeds: {ok_seeds}")
    print(f"Aggregate method: {args.aggregate}, length={len(agg)}")

if __name__ == "__main__":
    main()