# benchmarks/run_suite.py
import os, re, subprocess
import numpy as np
import pandas as pd

COMPILER = "./build/thermolangc"

BASE_MODES = ["off", "degree", "degree+variance"]   # normal runs
STRENGTHS = [float(x) for x in os.getenv("SUITE_STRENGTHS", "1.0").split(",")]                                  # expand later if needed

VAR_DIR = "results/variances"
PROFILE_SEEDS = 10
PROFILE_MODE = "off"          # profiling dynamics: "off" recommended
PROFILE_AGG = "mean"          # "mean" or "median"

# Variance shaping behavior for variance_profiled compile
VAR_POLICY = os.getenv("NOISE_SHAPING_VARIANCE_POLICY", "cool")
VAR_RENORM = os.getenv("NOISE_SHAPING_VARIANCE_RENORM", "1")
VAR_CAP    = os.getenv("NOISE_SHAPING_VARIANCE_SHRINK", "0.10")


def parse_final_state(stdout: str):
    m_state = re.search(r"\[FINAL_STATE\]:\s*\[(.*?)\]", stdout)
    if not m_state:
        return None
    return [int(x.strip()) for x in m_state.group(1).split(",")]

def parse_internal_energy(stdout: str):
    m_e = re.search(r"\[FINAL_ENERGY\]:\s*([-\d\.eE]+)", stdout)
    return float(m_e.group(1)) if m_e else None

def parse_seed_check(stdout: str):
    m = re.search(r"\[SEED_CHECK\]:\s*\[(.*?)\]", stdout)
    return m.group(1).strip() if m else None

def calc_true_energy(spins, J, h):
    s = np.array(spins[:len(h)], dtype=np.float32)
    if np.all((s == 0) | (s == 1)):
        s = 2.0 * s - 1.0
    return float(-0.5 * s.T @ J @ s - h.T @ s)

def compile_to_thrml(thermo_file: str, mode: str, strength: float = 1.0, extra_env=None):
    env = os.environ.copy()
    env["NOISE_SHAPING_MODE"] = mode
    env["NOISE_SHAPING_STRENGTH"] = str(strength)

    # default (can be overridden)
    env.setdefault("NOISE_SHAPING_VARIANCE_SHRINK", "0.5")

    if extra_env:
        env.update(extra_env)

    subprocess.run(
        [COMPILER, thermo_file, "--target=thrml"],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=True
    )

def patch_seed(py_file: str, seed: int):
    subprocess.run(["python3", "benchmarks/patch_seed.py", py_file, str(seed)], check=True)

def run_thrml(py_file: str, extra_env=None):
    env = os.environ.copy()
    if extra_env:
        env.update(extra_env)
    res = subprocess.run(["python3", py_file], capture_output=True, text=True, env=env)
    return res.stdout

def ensure_variance_file(tf: str):
    """
    Ensure results/variances/{problem}.csv exists.
    If missing, run benchmarks/collect_variance.py to generate it.
    """
    base = os.path.splitext(os.path.basename(tf))[0]
    out_csv = os.path.join(VAR_DIR, f"{base}.csv")
    if os.path.exists(out_csv):
        return out_csv

    os.makedirs(VAR_DIR, exist_ok=True)
    print(f"[INFO] Variance file missing for {base}. Generating profile -> {out_csv}")

    subprocess.run(
        [
            "python3", "benchmarks/collect_variance.py",
            "--thermo", tf,
            "--seeds", str(PROFILE_SEEDS),
            "--mode", PROFILE_MODE,
            "--aggregate", PROFILE_AGG,
            "--outdir", VAR_DIR
        ],
        check=True
    )
    if not os.path.exists(out_csv):
        raise RuntimeError(f"Variance collector did not produce {out_csv}")
    return out_csv

def load_variance_string(csv_path: str, n_inject: int):
    """
    Read first line of CSV (comma-separated floats), trim to n_inject.
    This is KEY: we inject ONLY logical spins (len(h_true)), not ancillas.
    """
    with open(csv_path, "r") as f:
        line = f.readline().strip()
    parts = [p.strip() for p in line.split(",") if p.strip() != ""]
    vals = [float(x) for x in parts]

    if len(vals) < n_inject:
        raise RuntimeError(f"Variance file too short: {csv_path}, got={len(vals)}, need={n_inject}")

    vals = vals[:n_inject]
    return ",".join(f"{x:.6f}" for x in vals)

def main(seeds=10, only_problem=None):
    subprocess.run(["python3", "benchmarks/generate_suite.py"], check=True)

    thermo_files = sorted([
        os.path.join("examples", f) for f in os.listdir("examples")
        if f.endswith(".thermo") and os.path.exists(os.path.join("examples", f.replace(".thermo", ".npz")))
    ])

    rows = []
    for tf in thermo_files:
        base = os.path.splitext(os.path.basename(tf))[0]
        if only_problem and base != only_problem:
            continue

        npz_path = tf.replace(".thermo", ".npz")
        data = np.load(npz_path)
        J_true, h_true = data["J"], data["h"]
        py_file = f"{base}_thrml.py"

        # -------------------------
        # 1) Normal mode runs
        # -------------------------
        for mode in BASE_MODES:
            for strength in STRENGTHS:
                for seed in range(seeds):
                    compile_to_thrml(tf, mode, strength=strength)
                    patch_seed(py_file, seed)
                    out = run_thrml(py_file, extra_env={"THERMOLANG_PROFILE_VARIANCE": "0"})

                    spins = parse_final_state(out)
                    if spins is None:
                        rows.append({"problem": base, "mode": mode, "strength": strength, "seed": seed, "status": "parse_fail"})
                        continue

                    internal_e = parse_internal_energy(out)
                    true_e = calc_true_energy(spins, J_true, h_true)
                    seed_check = parse_seed_check(out)

                    rows.append({
                        "problem": base, "mode": mode, "strength": strength, "seed": seed,
                        "true_energy": true_e, "internal_energy": internal_e,
                        "seed_check": seed_check, "status": "ok"
                    })
                    print(f"[OK] {base:22s} mode={mode:18s} λ={strength:4.2f} seed={seed:2d} trueE={true_e:.4f}")

        # -------------------------
        # 2) Variance-profiled runs (2-phase)
        # -------------------------
        var_csv = ensure_variance_file(tf)

        # Inject only logical spins (len(h_true)), NOT ancillas
        var_str = load_variance_string(var_csv, n_inject=len(h_true))

        var_env = {
            "NOISE_SHAPING_VARIANCES": var_str,
            "NOISE_SHAPING_VARIANCE_SHRINK": VAR_CAP,            # cap for policy
            "NOISE_SHAPING_VARIANCE_POLICY": VAR_POLICY,         # cool vs warm
            "NOISE_SHAPING_VARIANCE_RENORM": VAR_RENORM,         # keep mean beta stable
        }
        for strength in STRENGTHS:
            for seed in range(seeds):
                for compile_mode, label in [
                    ("variance", "variance_profiled"),
                    ("degree+variance", "degree+variance_profiled"),
                ]:
                    compile_to_thrml(tf, compile_mode, strength=1.0, extra_env=var_env)
                    patch_seed(py_file, seed)
                    out = run_thrml(py_file, extra_env={"THERMOLANG_PROFILE_VARIANCE": "0"})

                    spins = parse_final_state(out)
                    if spins is None:
                        rows.append({"problem": base, "mode": label, "strength": 1.0, "seed": seed, "status": "parse_fail"})
                        continue

                    internal_e = parse_internal_energy(out)
                    true_e = calc_true_energy(spins, J_true, h_true)
                    seed_check = parse_seed_check(out)

                    rows.append({
                        "problem": base, "mode": label, "strength": 1.0, "seed": seed,
                        "true_energy": true_e, "internal_energy": internal_e,
                        "seed_check": seed_check, "status": "ok"
                    })
                    print(f"[OK] {base:22s} mode={label:18s} seed={seed:2d} trueE={true_e:.4f}")

    df = pd.DataFrame(rows)
    os.makedirs("results", exist_ok=True)
    
    print("\nSaved: results/suite_raw.csv")

    ok = df[df["status"] == "ok"].copy()
    summary = (ok.groupby(["problem", "mode", "strength"])
                 .agg(true_mean=("true_energy", "mean"),
                      true_std=("true_energy", "std"),
                      true_min=("true_energy", "min"),
                      internal_mean=("internal_energy", "mean"))
                 .reset_index())
    OUT_CSV = os.getenv("SUITE_OUT_CSV", "results/suite_raw.csv")
    OUT_SUM = os.getenv("SUITE_OUT_SUMMARY", "results/suite_summary.csv")
    df.to_csv(OUT_CSV, index=False)
    summary.to_csv(OUT_SUM, index=False)
    print(f"\nSaved: {OUT_CSV}")
    print(f"Saved: {OUT_SUM}")
    print("Saved: results/suite_summary.csv")
    print(summary)


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", type=int, default=10)
    ap.add_argument("--only", type=str, default=None,
                    help="Run only this benchmark (basename without extensions)")
    args = ap.parse_args()
    main(seeds=args.seeds, only_problem=args.only)
