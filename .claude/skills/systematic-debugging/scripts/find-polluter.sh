#!/usr/bin/env bash
# Find which test creates unwanted file or directory state.
#
# Usage:
#   find-polluter.sh <path_to_check> <test_source_dir> [build_dir]
#
# Example:
#   .claude/skills/systematic-debugging/scripts/find-polluter.sh \
#     reports/report.xml submission/test _build/debug

set -euo pipefail

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "usage: $0 <path_to_check> <test_source_dir> [build_dir]" >&2
  exit 2
fi

pollution_check=$1
test_source_dir=$2
build_dir=${3:-_build/debug}

if [[ ! -d "$build_dir" ]]; then
  echo "error: build directory not found: $build_dir" >&2
  echo "run ./build.sh first" >&2
  exit 1
fi

mapfile -t test_files < <(find "$test_source_dir" -name "*.cpp" -type f | sort)

echo "checking for polluter of: $pollution_check"
echo "test source dir: $test_source_dir"
echo "build dir: $build_dir"
echo "test files: ${#test_files[@]}"

for test_file in "${test_files[@]}"; do
  if [[ -e "$pollution_check" ]]; then
    echo "warning: pollution already exists before $test_file; skipping" >&2
    continue
  fi

  test_name=$(basename "$test_file" .cpp)
  test_pattern=${test_name#test_}

  echo "running candidate: $test_file"
  ctest --test-dir "$build_dir" -R "$test_pattern" --output-on-failure >/dev/null 2>&1 || true

  if [[ -e "$pollution_check" ]]; then
    echo
    echo "found polluter"
    echo "test file: $test_file"
    echo "ctest pattern: $test_pattern"
    echo "created: $pollution_check"
    ls -la "$pollution_check"
    echo
    echo "re-run with:"
    echo "ctest --test-dir $build_dir -R $test_pattern -V"
    exit 1
  fi
done

echo "no polluter found"
