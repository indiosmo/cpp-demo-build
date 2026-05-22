#!/usr/bin/env bash
# Purpose: Activates the repository's local C++ toolchain environment.
# Usage:   source scripts/setenv.sh
# Notes:   Source this before running CMake directly. build.sh sources it
#          automatically when needed.
set -euo pipefail
shopt -s inherit_errexit

readonly SETENV_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly REPO_ROOT="$(cd "$SETENV_DIR/.." && pwd)"

# shellcheck source=versions.sh
source "$SETENV_DIR/versions.sh"
# shellcheck source=setpath.sh
source "$SETENV_DIR/setpath.sh"

# Keep compiler caches under the build tree so sandboxed builds do not need
# write access to user-level cache directories.
if command -v ccache > /dev/null 2>&1; then
  export CCACHE_DIR="${CCACHE_DIR:-$REPO_ROOT/_build/.ccache}"
  export CCACHE_TEMPDIR="${CCACHE_TEMPDIR:-$CCACHE_DIR/tmp}"
  mkdir -p "$CCACHE_DIR" "$CCACHE_TEMPDIR"
fi

export ASAN_OPTIONS="${ASAN_OPTIONS:-color=always:print_legend=0:abort_on_error=1:detect_invalid_pointer_pairs=2:check_initialization_order=1:strict_string_checks=1:detect_stack_use_after_return=1}"
export TSAN_OPTIONS="${TSAN_OPTIONS:-verbosity=1 history_size=7 second_deadlock_stack=1}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-print_stacktrace=1}"

export KRAKEN_ENV_SOURCED=1
