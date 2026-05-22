#!/usr/bin/env bash
# Build with a local CMake preset and run the black-box UDP harness without Docker.
#
# Usage:
#   run_local_submission [debug|release|asan|tsan|clang]

set -euo pipefail
shopt -s inherit_errexit

usage() {
  echo "usage: run_local_submission [debug|release|asan|tsan|clang]"
  echo "       default preset: debug"
}

main() {
  local preset="${1:-debug}"

  if (($# > 1)); then
    echo "too many arguments" >&2
    usage >&2
    return 2
  fi

  if [[ $preset == "-h" || $preset == "--help" ]]; then
    usage
    return 0
  fi

  case $preset in
    debug|release|asan|tsan|clang) ;;
    *)
      echo "invalid preset: ${preset} (expected debug, release, asan, tsan, or clang)" >&2
      return 2
      ;;
  esac

  local script_dir
  script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

  local bin="${script_dir}/_build/${preset}/kraken_submission"
  local harness_dir="${script_dir}/test"
  local reports_dir="${script_dir}/reports"

  mkdir -p "$reports_dir"

  "$script_dir/build.sh" "$preset"

  rm -f "$harness_dir/report.xml" "$harness_dir/report.html"

  set +e
  "$harness_dir/run_tests.sh" --mode udp --bin "$bin"
  local test_status=$?
  set -e

  for report in report.xml report.html; do
    if [[ -f "$harness_dir/$report" ]]; then
      rm -f "$reports_dir/$report"
      cp "$harness_dir/$report" "$reports_dir/$report"
    fi
  done

  rm -f "$harness_dir/report.xml" "$harness_dir/report.html"

  return "$test_status"
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
