#!/usr/bin/env bash
# Purpose: Sets up the local development environment for this repository.
# Usage:   setup.sh [--skip-system-packages]
# Notes:   Installs shared developer tools, local C++ toolchain dependencies,
#          CMake user presets, and repository hooks.
set -euo pipefail
shopt -s inherit_errexit

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck source=scripts/install_uv.sh
source "$SCRIPT_DIR/scripts/install_uv.sh"

main() {
  local -a dependency_args=()

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --skip-system-packages)
        dependency_args+=("$1")
        shift
        ;;
      -h|--help)
        printf '%s\n' "usage: setup.sh [--skip-system-packages]"
        return 0
        ;;
      *)
        printf '%s\n' "unknown argument: $1" >&2
        printf '%s\n' "usage: setup.sh [--skip-system-packages]" >&2
        return 2
        ;;
    esac
  done

  printf '%s\n' "Setting up matching-engine-lab development environment..."

  git -C "$SCRIPT_DIR" submodule update --init --recursive
  "$SCRIPT_DIR/scripts/install_dependencies.sh" "${dependency_args[@]}"
  ensure_uv
  "$SCRIPT_DIR/scripts/install_precommit_hooks.sh"
  "$SCRIPT_DIR/scripts/generate-cmake-user-presets.sh"

  printf '%s\n' "Setup complete. Run 'source scripts/setenv.sh' before direct CMake commands."
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
