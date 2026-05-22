#!/usr/bin/env bash
# Purpose: Refresh the cached google-benchmark JSON that
#          render_performance_charts.py consumes. Writes a single JSON
#          under docs/performance/data/.
# Usage:   collect_performance_data.sh [BUILD_DIR]
# Notes:   Repetitions and percentile statistics are baked into the
#          benchmark registration (current_latency_percentiles). The
#          registrations use Iterations(K) so MIN_TIME has no effect on
#          the per-repetition iteration count; it is left in for parity
#          with other google-benchmark drivers.
set -euo pipefail
shopt -s inherit_errexit

die() { printf '%s\n' "$*" >&2; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
readonly SCRIPT_DIR PROJECT_ROOT

BUILD_DIR="${1:-$PROJECT_ROOT/_build/release}"
DATA_DIR="$PROJECT_ROOT/docs/performance/data"
BENCH_BINARY="$BUILD_DIR/submission/benchmarks/order_book/matching_engine_order_book_benchmark"

MIN_TIME="${MIN_TIME:-0.5s}"

[[ -x "$BENCH_BINARY" ]] || die "missing binary: $BENCH_BINARY
build first with:
  ./build.sh release matching_engine_order_book_benchmark"

mkdir -p "$DATA_DIR"

echo "Running matching_engine_order_book_benchmark (min_time=$MIN_TIME)..."
"$BENCH_BINARY" \
  --benchmark_filter='current_latency_percentiles' \
  --benchmark_min_time="$MIN_TIME" \
  --benchmark_report_aggregates_only=true \
  --benchmark_format=json \
  --benchmark_out="$DATA_DIR/benchmark.json" \
  > /dev/null

echo
echo "Wrote cached data to $DATA_DIR:"
ls -lh "$DATA_DIR" | awk 'NR>1 {printf "  %-30s %s\n", $NF, $5}'
echo
echo "Render charts with:"
echo "  ./scripts/render_performance_charts.py"
