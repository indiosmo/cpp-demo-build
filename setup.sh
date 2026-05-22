#!/usr/bin/env bash
# Purpose: Sets up the local development environment for this repository.
# Usage:   setup.sh
# Notes:   Installs shared developer tools, local C++ toolchain dependencies,
#          CMake user presets, and repository hooks.
set -euo pipefail
shopt -s inherit_errexit

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck source=scripts/install_uv.sh
source "$SCRIPT_DIR/scripts/install_uv.sh"

main() {
  printf '%s\n' "Setting up kraken-submission development environment..."

  git -C "$SCRIPT_DIR" submodule update --init --recursive
  "$SCRIPT_DIR/scripts/install_dependencies.sh"
  ensure_uv
  "$SCRIPT_DIR/scripts/install_precommit_hooks.sh"
  "$SCRIPT_DIR/scripts/generate-cmake-user-presets.sh"

  printf '%s\n' "Setup complete. Run 'source scripts/setenv.sh' before direct CMake commands."
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
