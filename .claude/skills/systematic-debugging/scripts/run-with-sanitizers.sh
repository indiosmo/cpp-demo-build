#!/usr/bin/env bash
# Rebuild and run tests under an available sanitizer preset.
#
# Usage:
#   run-with-sanitizers.sh [asan|tsan] [ctest_pattern]

set -euo pipefail

sanitizer=${1:-asan}
pattern=${2:-}

case "$sanitizer" in
  asan|tsan) ;;
  *)
    echo "error: unknown sanitizer preset: $sanitizer" >&2
    echo "valid presets: asan, tsan" >&2
    exit 2
    ;;
esac

./build.sh "$sanitizer"

build_dir="_build/$sanitizer"
if [[ ! -d "$build_dir" ]]; then
  echo "error: build directory not found: $build_dir" >&2
  exit 1
fi

if [[ -n "$pattern" ]]; then
  ctest --test-dir "$build_dir" -R "$pattern" --output-on-failure
else
  ctest --test-dir "$build_dir" --output-on-failure
fi
