#!/usr/bin/env bash
# Purpose: Sets up the local development environment for this repository.
# Usage:   setup.sh
# Notes:   Installs shared developer tools and configures repository hooks.
set -euo pipefail
shopt -s inherit_errexit

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck source=scripts/install_uv.sh
source "$SCRIPT_DIR/scripts/install_uv.sh"

main() {
  printf '%s\n' "Setting up kraken-submission development environment..."

  ensure_uv
  "$SCRIPT_DIR/scripts/install_precommit_hooks.sh"

  printf '%s\n' "Setup complete."
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
