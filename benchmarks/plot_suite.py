# benchmarks/plot_suite.py
import os
import pandas as pd
import matplotlib.pyplot as plt

def main():
    df = pd.read_csv("results/suite_summary.csv")
    os.makedirs("results/plots", exist_ok=True)

    for problem in sorted(df["problem"].unique()):
        sub = df[df["problem"] == problem].copy()

        modes = sub["mode"].tolist()
        means = sub["true_mean"].tolist()
        stds  = sub["true_std"].fillna(0.0).tolist()

        plt.figure(figsize=(9, 5))
        plt.bar(modes, means, yerr=stds, capsize=4)
        plt.title(f"Noise Shaping Comparison (True Energy)\n{problem}")
        plt.ylabel("True Energy (lower is better)")
        plt.grid(axis="y", alpha=0.3)
        plt.tight_layout()
        out = f"results/plots/{problem}_true_energy.png"
        plt.savefig(out)
        plt.close()
        print("Saved:", out)

    # Also produce a global "improvement vs off" table plot (optional)
    # Build improvement = off_mean - best_mean
    piv = df.pivot_table(index="problem", columns="mode", values="true_mean", aggfunc="first")
    if "off" in piv.columns:
        piv["best"] = piv.min(axis=1)
        piv["improvement"] = piv["off"] - piv["best"]
        piv = piv.sort_values("improvement", ascending=False)

        plt.figure(figsize=(10, 5))
        plt.bar(piv.index.tolist(), piv["improvement"].tolist())
        plt.title("Best-Mode Improvement over Baseline (off)")
        plt.ylabel("Improvement (off_mean - best_mean)")
        plt.xticks(rotation=30, ha="right")
        plt.grid(axis="y", alpha=0.3)
        plt.tight_layout()
        out = "results/plots/global_improvement.png"
        plt.savefig(out)
        plt.close()
        print("Saved:", out)

if __name__ == "__main__":
    main()
