"""
Thread scaling benchmark for the Collision Sandbox.

Runs build/collision_sandbox.exe once per thread count, reads the timings it
prints, and plots how much faster each broadphase gets as threads are added.
The measured speedup is compared against two simple models:

  * Ideal linear scaling:  speedup(p) = p
  * Amdahl's law:          speedup(p) = 1 / ((1 - f) + f / p)
    where f is the fraction of the work that can run in parallel.
    f is fitted to the measurements.

Usage:
    py benchmark_threads.py                 # defaults: 2000 and 10000 spheres, 1..16 threads
    py benchmark_threads.py --spheres 5000  # one sphere count only
    py benchmark_threads.py --max-threads 8 --repeats 5

Output:
    benchmark_results.csv   raw numbers, one row per run
    benchmark_threads.png   the plot
"""

import argparse
import os
import re
import subprocess
import sys

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# ---------------------------------------------------------------------------
# Settings
# ---------------------------------------------------------------------------

DEFAULT_SPHERE_COUNTS = [2000, 10000]
DEFAULT_MAX_THREADS = os.cpu_count() or 8
DEFAULT_REPEATS = 3          # runs per configuration; the median is kept
FRAMES_PER_RUN = 30          # keeps each run short
EXE_PATH = os.path.join("build", "collision_sandbox.exe")

# Lines printed by the sandbox look like:
#   brute force, 16 thread(s):    24.21 ms, 1452 pairs
#   spatial grid,  1 thread(s):    4.28 ms, 1452 pairs
#   average frame time: 2.650 ms
LINE_PATTERN = re.compile(r"(brute force|spatial grid),\s+(\d+) thread\(s\):\s+([\d.]+) ms")
FRAME_PATTERN = re.compile(r"average frame time:\s+([\d.]+) ms")


# ---------------------------------------------------------------------------
# Running the sandbox and parsing what it prints
# ---------------------------------------------------------------------------

def run_sandbox(sphere_count, thread_count):
    """Run the exe once and return a dict of the timings it printed."""
    command = [EXE_PATH, str(sphere_count), str(FRAMES_PER_RUN), str(thread_count)]
    result = subprocess.run(command, capture_output=True, text=True, check=True)

    timings = {}
    for method, threads, ms in LINE_PATTERN.findall(result.stdout):
        # The exe always prints a 1-thread line and a thread_count line.
        # We only keep the line that matches the thread count we asked for.
        if int(threads) == thread_count:
            timings[method] = float(ms)

    frame_match = FRAME_PATTERN.search(result.stdout)
    if frame_match:
        timings["full frame"] = float(frame_match.group(1))

    if len(timings) != 3:
        raise RuntimeError(f"Could not parse sandbox output:\n{result.stdout}")
    return timings


def collect_results(sphere_counts, max_threads, repeats):
    """Run every configuration and return a tidy DataFrame with one row per run."""
    rows = []
    thread_counts = range(1, max_threads + 1)
    total_runs = len(sphere_counts) * len(thread_counts) * repeats
    done = 0

    for spheres in sphere_counts:
        for threads in thread_counts:
            for repeat in range(repeats):
                timings = run_sandbox(spheres, threads)
                for method, ms in timings.items():
                    rows.append({
                        "spheres": spheres,
                        "threads": threads,
                        "repeat": repeat,
                        "method": method,
                        "ms": ms,
                    })
                done += 1
                print(f"\r  {done}/{total_runs} runs done", end="", flush=True)
    print()
    return pd.DataFrame(rows)


# ---------------------------------------------------------------------------
# Modelling
# ---------------------------------------------------------------------------

def amdahl_speedup(threads, parallel_fraction):
    """Amdahl's law: the serial part never gets faster, only the parallel part does."""
    serial = 1.0 - parallel_fraction
    return 1.0 / (serial + parallel_fraction / threads)


def fit_parallel_fraction(threads, measured_speedup):
    """
    Find the parallel fraction f that makes Amdahl's law best match the data.
    Very simple: try 1000 values of f between 0 and 1, keep the one with the
    smallest squared error. No curve-fitting library needed.
    """
    candidates = np.linspace(0.0, 1.0, 1001)
    errors = []
    for f in candidates:
        predicted = amdahl_speedup(threads, f)
        errors.append(np.sum((predicted - measured_speedup) ** 2))
    return candidates[int(np.argmin(errors))]


def summarise(df):
    """
    Collapse repeats to the median, then add speedup relative to 1 thread.
    Returns one row per (spheres, method, threads).
    """
    median = (df.groupby(["spheres", "method", "threads"])["ms"]
                .median()
                .reset_index())

    # Time at 1 thread for each (spheres, method), to divide by.
    baseline = (median[median["threads"] == 1]
                .set_index(["spheres", "method"])["ms"]
                .rename("ms_1_thread"))
    median = median.join(baseline, on=["spheres", "method"])
    median["speedup"] = median["ms_1_thread"] / median["ms"]
    return median


# ---------------------------------------------------------------------------
# Plotting
# ---------------------------------------------------------------------------

def plot_results(summary, output_path):
    sphere_counts = sorted(summary["spheres"].unique())
    methods = ["brute force", "spatial grid", "full frame"]

    fig, axes = plt.subplots(2, len(sphere_counts), figsize=(6 * len(sphere_counts), 9), squeeze=False)

    for column, spheres in enumerate(sphere_counts):
        subset = summary[summary["spheres"] == spheres]
        threads = np.arange(1, subset["threads"].max() + 1)

        # Top row: raw time in milliseconds.
        ax_time = axes[0][column]
        for method in methods:
            data = subset[subset["method"] == method]
            ax_time.plot(data["threads"], data["ms"], marker="o", label=method)
        ax_time.set_title(f"{spheres} spheres: time per call")
        ax_time.set_xlabel("threads")
        ax_time.set_ylabel("milliseconds (median)")
        ax_time.set_yscale("log")
        ax_time.grid(True, which="both", alpha=0.3)
        ax_time.legend()

        # Bottom row: speedup vs. the two models.
        ax_speed = axes[1][column]
        ax_speed.plot(threads, threads, linestyle="--", color="gray", label="ideal (linear)")
        for method in methods:
            data = subset[subset["method"] == method]
            f = fit_parallel_fraction(data["threads"].to_numpy(), data["speedup"].to_numpy())
            line, = ax_speed.plot(data["threads"], data["speedup"], marker="o",
                                  label=f"{method} (measured)")
            ax_speed.plot(threads, amdahl_speedup(threads, f), linestyle=":",
                          color=line.get_color(), label=f"{method} Amdahl fit, f={f:.2f}")
        ax_speed.set_title(f"{spheres} spheres: speedup vs 1 thread")
        ax_speed.set_xlabel("threads")
        ax_speed.set_ylabel("speedup")
        ax_speed.grid(True, alpha=0.3)
        ax_speed.legend(fontsize=8)

    fig.suptitle("Collision Sandbox thread scaling", fontsize=14)
    fig.tight_layout()
    fig.savefig(output_path, dpi=120)
    print(f"Saved plot to {output_path}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(description="Benchmark the sandbox across thread counts.")
    parser.add_argument("--spheres", type=int, nargs="+", default=DEFAULT_SPHERE_COUNTS,
                        help="one or more sphere counts to test")
    parser.add_argument("--max-threads", type=int, default=DEFAULT_MAX_THREADS,
                        help="test 1..N threads")
    parser.add_argument("--repeats", type=int, default=DEFAULT_REPEATS,
                        help="runs per configuration, median is used")
    return parser.parse_args()


def main():
    args = parse_args()

    if not os.path.exists(EXE_PATH):
        print(f"Cannot find {EXE_PATH}. Run build.bat first.")
        return 1

    print(f"Testing {args.spheres} spheres on 1..{args.max_threads} threads, "
          f"{args.repeats} repeats each")
    raw = collect_results(args.spheres, args.max_threads, args.repeats)
    raw.to_csv("benchmark_results.csv", index=False)
    print("Saved raw results to benchmark_results.csv")

    summary = summarise(raw)

    # Print a small table: speedup at the highest thread count for each case.
    best = summary[summary["threads"] == args.max_threads]
    print(f"\nSpeedup at {args.max_threads} threads:")
    print(best.pivot(index="spheres", columns="method", values="speedup").round(2).to_string())

    plot_results(summary, "benchmark_threads.png")
    return 0


if __name__ == "__main__":
    sys.exit(main())
