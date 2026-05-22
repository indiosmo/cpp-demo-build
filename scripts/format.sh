#!/bin/bash

# Format C++ files in parallel using all available cores, 30 files per batch.
# Usage: format.sh [--all]
#   --all  Format all tracked C++ files (default: only files changed vs HEAD)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$PROJECT_ROOT"

PATHS=(src/ test/)
EXCLUDES=(':(exclude)vendor/')

if [[ "${1:-}" == "--all" ]]; then
  git ls-files "${PATHS[@]}" "${EXCLUDES[@]}" | grep -E "\.(cpp|hpp|h)$"
else
  git diff --name-only --diff-filter=d HEAD -- "${PATHS[@]}" "${EXCLUDES[@]}" | grep -E "\.(cpp|hpp|h)$"
fi | xargs -r -P "$(nproc)" -n 30 clang-format -i --style=file
