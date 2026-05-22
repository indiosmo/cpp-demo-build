#!/usr/bin/env bash
# Configure, build, and test the submission using a CMake preset.
#
# Usage:
#   build.sh [preset] [target] [cmake_option ...]
#
# Examples:
#   build.sh                                            # debug preset, all targets
#   build.sh asan                                       # asan preset, all targets
#   build.sh release kraken_submission                  # build a single target
#   build.sh debug -DKRAKEN_BUILD_BENCHMARKS=ON         # extra configure options
#   build.sh release kraken_submission -DKRAKEN_BUILD_BENCHMARKS=ON -DKRAKEN_FOO=OFF
#
# Anything after the (optional) target that begins with '-' is forwarded to the
# configure step. Tests always run after a successful build.
#
# Available presets: debug (default), release, asan, tsan, clang.

set -euo pipefail
shopt -s inherit_errexit

usage() {
  echo "usage: build.sh [debug|release|asan|tsan|clang] [target] [cmake_option ...]"
  echo "       default preset: debug"
}

main() {
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

  local script_dir
  script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

  local num_cores
  num_cores=$(grep -c '^processor' /proc/cpuinfo)

  cmake --preset="$preset" -S "$script_dir" "${cmake_options[@]}"
  cmake --build "$script_dir/_build/$preset" --target "${target:-all}" --parallel "$num_cores"

  ctest --test-dir "$script_dir/_build/$preset" --parallel "$num_cores" --output-on-failure

  cp "$script_dir/_build/$preset/compile_commands.json" "$script_dir/compile_commands.json" 2>/dev/null || true

  # Keep _build/clang's compile DB fresh so clangd (which reads from there per
  # .clangd) sees current targets and includes without a separate build.
  if [[ "$preset" != "clang" ]]; then
    cmake --preset=clang -S "$script_dir" >/dev/null
  fi
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
