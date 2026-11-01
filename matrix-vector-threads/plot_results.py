"""
Draws the execution-time charts from results.csv (produced by the benchmark).

    python3 plot_results.py            # reads results.csv, writes chart.png
"""
import csv
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def load(path):
    with open(path) as f:
        rows = list(csv.DictReader(f))
    thread_counts = [int(c.split("_")[1]) for c in rows[0] if c.startswith("parallel_")]
    data = {}
    for row in rows:
        n = int(row["n"])
        data[n] = {
            "sequential": int(row["sequential_us"]),
            "parallel": [int(row[f"parallel_{t}"]) for t in thread_counts],
        }
    return thread_counts, data


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "results.csv"
    thread_counts, data = load(path)

    fig, ax = plt.subplots(figsize=(9, 5.5))
    for n in sorted(data):
        ax.plot(thread_counts, data[n]["parallel"], marker="o", label=f"n = {n}")

    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xticks(thread_counts)
    ax.set_xticklabels(thread_counts)
    ax.set_xlabel("Number of threads")
    ax.set_ylabel("Execution time, microseconds (log scale)")
    ax.set_title("Matrix-vector multiplication: time vs thread count")
    ax.grid(True, which="both", linewidth=0.3)
    ax.legend()

    fig.tight_layout()
    fig.savefig("chart.png", dpi=130)
    print("Wrote chart.png")


if __name__ == "__main__":
    main()
