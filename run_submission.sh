#!/usr/bin/env bash
# Build the docker image and run the kraken interview test suite end-to-end,
# mirroring the workflow described in README.md.
#
# Usage:
#   run_submission.sh [-t image_tag] [-m stdin|udp]
#
# Reports are written to ./reports/ relative to this script.

set -euo pipefail
shopt -s inherit_errexit

usage() {
  echo "usage: run_submission.sh [-t image_tag] [-m stdin|udp]"
}

main() {
  local image_tag="kraken-orderbook-senior"
  local test_mode="udp"

  while (($# > 0)); do
    case $1 in
      -t|--tag)
        (($# >= 2)) || { echo "missing value for $1" >&2; usage >&2; return 2; }
        image_tag=$2; shift 2 ;;
      -m|--mode)
        (($# >= 2)) || { echo "missing value for $1" >&2; usage >&2; return 2; }
        test_mode=$2; shift 2 ;;
      -h|--help) usage; return 0 ;;
      *) echo "unknown argument: $1" >&2; usage >&2; return 2 ;;
    esac
  done

  if [[ $test_mode != "stdin" && $test_mode != "udp" ]]; then
    echo "invalid mode: ${test_mode} (expected stdin or udp)" >&2
    return 2
  fi

  local script_dir
  script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
  local reports_dir="${script_dir}/reports"

  mkdir -p "$reports_dir"

  docker build -t "$image_tag" "$script_dir"

  docker run --rm \
    -e "TEST_MODE=${test_mode}" \
    -v "${reports_dir}:/reports" \
    "$image_tag"
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
