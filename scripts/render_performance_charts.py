#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = [
#   "matplotlib>=3.8",
#   "pandas>=2.1",
#   "seaborn>=0.13",
# ]
# ///
"""
Render the v3 order book performance chart that docs/performance.md links to.

Reads cached benchmark output from docs/performance/data/benchmark.json and
writes one SVG chart to docs/performance/. The JSON is produced by
scripts/collect_performance_data.sh; this script does not invoke the C++
binary itself.

Chart:
    latency_percentiles.svg p50/p90/p95/p99 for insert and cancel at depth 200

Usage:
    ./scripts/render_performance_charts.py [--data-dir docs/performance/data]

Requires uv (https://astral.sh/uv). Dependencies are declared inline (PEP 723)
and resolved into an ephemeral environment by uv on first run.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns

PROJECT_ROOT = Path(__file__).resolve().parent.parent
OUTPUT_DIR = PROJECT_ROOT / "docs" / "performance"
DATA_DIR = OUTPUT_DIR / "data"
BENCH_JSON_NAME = "benchmark.json"

PERCENTILE_DEPTH = 200
PERCENTILE_AGGREGATES = ["p50", "p90", "p95", "p99"]

# Map google-benchmark function family to a friendly axis label.
OPERATION_LABEL = {
    "BM_PlaceGrowingLevel": "insert",
    "BM_CancelDrainingLevel": "cancel",
}

PALETTE = {"insert": "#1f77b4", "cancel": "#d62728"}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--data-dir",
        type=Path,
        default=DATA_DIR,
        help="Directory containing benchmark.json (default: %(default)s)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=OUTPUT_DIR,
        help="Output directory for the SVG chart (default: %(default)s)",
    )
    return parser.parse_args()


def load_records(json_path: Path) -> pd.DataFrame:
    """Flatten the google-benchmark JSON into a long-form DataFrame.

    One row per (operation, depth, aggregate) tuple. The current benchmark
    registration uses Iterations(K) with K equal to the level depth and one
    operation per iteration, so the cpu_time aggregate is per-op directly;
    depth is parsed from the iterations:<K> segment and used only as a label
    on the percentile chart.
    """
    with json_path.open() as fh:
        payload = json.load(fh)

    rows: list[dict] = []
    for entry in payload["benchmarks"]:
        if entry.get("run_type") != "aggregate":
            continue
        family = entry["name"].split("/", 1)[0]
        operation = OPERATION_LABEL.get(family)
        if operation is None:
            continue
        # Repetition-aware name shape:
        #   BM_X/current_latency_percentiles/iterations:<depth>/repeats:<N>_<aggregate>
        segments = entry["name"].split("/")
        depth: int | None = None
        for segment in segments:
            if segment.startswith("iterations:"):
                depth = int(segment.split(":", 1)[1])
                break
        if depth is None:
            continue
        rows.append(
            {
                "operation": operation,
                "depth": depth,
                "aggregate": entry["aggregate_name"],
                "per_op_ns": float(entry["cpu_time"]),
            }
        )

    if not rows:
        raise RuntimeError(
            f"{json_path} contained no current_latency_percentiles aggregate rows; "
            "re-run scripts/collect_performance_data.sh"
        )

    return pd.DataFrame(rows)


def render_percentile_chart(df: pd.DataFrame, output: Path) -> None:
    """Grouped bar chart of latency percentiles at the reference depth."""
    at_depth = df[
        (df["depth"] == PERCENTILE_DEPTH) & (df["aggregate"].isin(PERCENTILE_AGGREGATES))
    ].copy()
    if at_depth.empty:
        raise RuntimeError(
            f"no percentile rows at depth {PERCENTILE_DEPTH}; "
            "did the benchmark register ComputeStatistics?"
        )
    at_depth["aggregate"] = pd.Categorical(
        at_depth["aggregate"], categories=PERCENTILE_AGGREGATES, ordered=True
    )
    at_depth = at_depth.sort_values(["aggregate", "operation"])

    fig, ax = plt.subplots(figsize=(7.5, 4.5), constrained_layout=True)
    sns.barplot(
        data=at_depth,
        x="aggregate",
        y="per_op_ns",
        hue="operation",
        palette=PALETTE,
        ax=ax,
    )
    for container in ax.containers:
        ax.bar_label(container, fmt="%.1f", padding=2, fontsize=9)
    ax.set_xlabel("percentile (across 30 repetitions)")
    ax.set_ylabel("per-op latency (ns)")
    ax.set_title(
        f"v3 flat_order_book: insert / cancel percentiles at depth {PERCENTILE_DEPTH}"
    )
    ax.legend(title="operation")
    fig.savefig(output, format="svg")
    plt.close(fig)
    print(f"wrote {output}")


def main() -> int:
    args = parse_args()

    bench_json = args.data_dir / BENCH_JSON_NAME
    if not bench_json.exists():
        print(
            f"error: {bench_json} not found.\n"
            "Refresh the cached benchmark data first:\n"
            "  ./scripts/collect_performance_data.sh",
            file=sys.stderr,
        )
        return 1

    sns.set_theme(style="whitegrid", context="notebook")
    df = load_records(bench_json)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    render_percentile_chart(df, args.output_dir / "latency_percentiles.svg")
    return 0


if __name__ == "__main__":
    sys.exit(main())
