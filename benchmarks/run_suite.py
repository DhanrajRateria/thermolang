# benchmarks/run_suite.py
import os, re, subprocess
import numpy as np
import pandas as pd

COMPILER = "./build/thermolangc"
MODES = ["off", "degree", "degree+variance"]  # we handle variance separately as variance_profiled

# Paper-friendly sweep (set to [0.0,0.25,0.5,0.75,1.0] later)
STRENGTHS = [1.0]

def parse_final_state(stdout: str):
    m_state = re.search(r"\[FINAL_STATE\]:\s*\[(.*?)\]", stdout)
    if not m_state:
        return None
    return [int(x.strip()) for x in m_state.group(1).split(",")]

def parse_internal_energy(stdout: str):
    m_e = re.search(r"\[FINAL_ENERGY\]:\s*([-\d\.eE]+)", stdout)
    return float(m_e.group(1)) if m_e else None

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

def calc_true_energy(spins, J, h):
    s = np.array(spins[:len(h)], dtype=np.float32)
    # Safety if ever 0/1 slips in
    if np.all((s == 0) | (s == 1)):
        s = 2.0 * s - 1.0
    return float(-0.5 * s.T @ J @ s - h.T @ s)

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

def patch_seed(py_file: str, seed: int):
    subprocess.run(["python3", "benchmarks/patch_seed.py", py_file, str(seed)], check=True)

def run_thrml(py_file: str, extra_env=None):
    env = os.environ.copy()
    if extra_env:
        env.update(extra_env)
    res = subprocess.run(["python3", py_file], capture_output=True, text=True, env=env)
    return res.stdout

def main(seeds=10):
    # 1) Generate benchmarks
    subprocess.run(["python3", "benchmarks/generate_suite.py"], check=True)

    thermo_files = sorted([
        os.path.join("examples", f) for f in os.listdir("examples")
        if f.endswith(".thermo") and os.path.exists(os.path.join("examples", f.replace(".thermo", ".npz")))
    ])

    rows = []
    for tf in thermo_files:
        base = os.path.splitext(os.path.basename(tf))[0]
        npz_path = tf.replace(".thermo", ".npz")
        data = np.load(npz_path)
        J_true, h_true = data["J"], data["h"]

        py_file = f"{base}_thrml.py"

        # ---- A) OFF baseline energy runs (normal schedule) ----
        for seed in range(seeds):
            compile_to_thrml(tf, "off", strength=1.0)
            patch_seed(py_file, seed)
            out_off = run_thrml(py_file, extra_env={"THERMOLANG_PROFILE_VARIANCE": "0"})

            spins = parse_final_state(out_off)
            if spins is None:
                rows.append({"problem": base, "mode": "off", "strength": 1.0, "seed": seed, "status": "parse_fail"})
                continue

            internal_e = parse_internal_energy(out_off)
            true_e = calc_true_energy(spins, J_true, h_true)
            seed_check = parse_seed_check(out_off)

            rows.append({
                "problem": base, "mode": "off", "strength": 1.0, "seed": seed,
                "true_energy": true_e, "internal_energy": internal_e,
                "seed_check": seed_check, "status": "ok"
            })
            print(f"[OK] {base:22s} mode=off               seed={seed:2d} trueE={true_e:.4f}")

        # ---- B) Variance profiling runs: OFF + profiling schedule ----
        # We use OFF so variances reflect the unshaped model dynamics.
        variances_by_seed = {}
        for seed in range(seeds):
            compile_to_thrml(tf, "off", strength=1.0)
            patch_seed(py_file, seed)
            out_prof = run_thrml(py_file, extra_env={"THERMOLANG_PROFILE_VARIANCE": "1"})

            v = parse_variances(out_prof)
            seed_check = parse_seed_check(out_prof)

            if v is None or len(v) < len(h_true):
                print(f"[WARN] {base:22s} variance profile missing/short for seed={seed}, got={None if v is None else len(v)}")
                variances_by_seed[seed] = None
                continue

            variances_by_seed[seed] = v[:len(h_true)]
            print(f"[OK] {base:22s} variance_profiled seed={seed:2d} seed_check=[{seed_check}]")

        # ---- C) Variance-shaped runs: compile with injected variances ----
        for seed in range(seeds):
            v = variances_by_seed.get(seed)
            if v is None:
                rows.append({"problem": base, "mode": "variance_profiled", "strength": 1.0, "seed": seed, "status": "no_variances"})
                continue

            var_str = ",".join(f"{x:.6f}" for x in v)
            compile_to_thrml(tf, "variance", strength=1.0, extra_env={"NOISE_SHAPING_VARIANCES": var_str})
            patch_seed(py_file, seed)
            out_var = run_thrml(py_file, extra_env={"THERMOLANG_PROFILE_VARIANCE": "0"})

            spins = parse_final_state(out_var)
            if spins is None:
                rows.append({"problem": base, "mode": "variance_profiled", "strength": 1.0, "seed": seed, "status": "parse_fail"})
                continue

            internal_e = parse_internal_energy(out_var)
            true_e = calc_true_energy(spins, J_true, h_true)
            seed_check = parse_seed_check(out_var)

            rows.append({
                "problem": base, "mode": "variance_profiled", "strength": 1.0, "seed": seed,
                "true_energy": true_e, "internal_energy": internal_e,
                "seed_check": seed_check, "status": "ok"
            })
            print(f"[OK] {base:22s} mode=variance_profiled seed={seed:2d} trueE={true_e:.4f}")

        # ---- D) Degree / Degree+Variance strength sweeps (optional, paper-friendly) ----
        for mode in ["degree", "degree+variance"]:
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

    df = pd.DataFrame(rows)
    os.makedirs("results", exist_ok=True)
    df.to_csv("results/suite_raw.csv", index=False)
    print("\nSaved: results/suite_raw.csv")

    ok = df[df["status"] == "ok"].copy()
    summary = (ok.groupby(["problem", "mode", "strength"])
                 .agg(true_mean=("true_energy", "mean"),
                      true_std=("true_energy", "std"),
                      true_min=("true_energy", "min"),
                      internal_mean=("internal_energy", "mean"))
                 .reset_index())
    summary.to_csv("results/suite_summary.csv", index=False)
    print("Saved: results/suite_summary.csv")
    print(summary)

if __name__ == "__main__":
    import sys
    seeds = 10
    if len(sys.argv) == 2:
        seeds = int(sys.argv[1])
    main(seeds=seeds)
