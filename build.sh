#!/usr/bin/env bash
# Purpose: Configures, builds, and tests the project using a CMake preset.
# Usage:   build.sh [preset] [target] [cmake_option ...]
#          Example: build.sh release server
# Notes:   Sources scripts/setenv.sh automatically so CMake sees the local
#          GCC 16 toolchain without requiring shell startup changes.
set -euo pipefail
shopt -s inherit_errexit

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  printf '%s\n' "usage: build.sh [debug|release|asan|tsan|clang] [target] [cmake_option ...]"
  printf '%s\n' "       default preset: debug"
}

activate_environment() {
  if [[ -n "${LAB_ENV_SOURCED:-}" ]]; then
    return
  fi

  # shellcheck source=scripts/setenv.sh
  source "$SCRIPT_DIR/scripts/setenv.sh"
}

main() {
  activate_environment

  local preset="debug"
  local target=""
  local -a cmake_options=()

  if (($# >= 1)); then
    case $1 in
      -h|--help) usage; return 0 ;;
      -*) ;;  # leave for cmake_options below
      *) preset=$1; shift ;;
    esac
  fi

  # Treat the next non-flag positional as the build target.
  if (($# >= 1)) && [[ $1 != -* ]]; then
    target=$1
    shift
  fi

  # Everything else is forwarded verbatim to the cmake configure step.
  cmake_options=("$@")

  local num_cores
  num_cores=$(grep -c '^processor' /proc/cpuinfo)

  cmake --fresh --preset="$preset" -S "$SCRIPT_DIR" "${cmake_options[@]}"
  cmake --build "$SCRIPT_DIR/_build/$preset" --target "${target:-all}" --parallel "$num_cores"

  ctest --test-dir "$SCRIPT_DIR/_build/$preset" --parallel "$num_cores" --output-on-failure

  cp "$SCRIPT_DIR/_build/$preset/compile_commands.json" "$SCRIPT_DIR/compile_commands.json" 2>/dev/null || true

  # Keep _build/clang's compile DB fresh so clangd (which reads from there per
  # .clangd) sees current targets and includes without a separate build.
  if [[ "$preset" != "clang" ]]; then
    cmake --fresh --preset=clang -S "$SCRIPT_DIR" >/dev/null
  fi
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
