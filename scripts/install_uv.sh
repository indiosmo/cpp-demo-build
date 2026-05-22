#!/usr/bin/env bash
# Purpose: Ensures uv is installed and discoverable for repository scripts.
# Usage:   install_uv.sh
# Notes:   Installs uv into the user's environment if missing.
set -euo pipefail
shopt -s inherit_errexit

install_uv_die() { printf '%s\n' "$*" >&2; exit 1; }

install_uv_confirm() {
  local prompt="$1"
  local response

  if [[ ! -t 0 ]]; then
    install_uv_die "$prompt (no TTY available for confirmation; re-run interactively)"
  fi

  read -r -p "$prompt [y/N] " response
  [[ "$response" =~ ^[Yy]([Ee][Ss])?$ ]]
}

ensure_uv() {
  if command -v uv &> /dev/null; then
    return
  fi

  install_uv_confirm "uv is not installed. Install it via https://astral.sh/uv/install.sh?" \
    || install_uv_die "uv is required; aborting"

  printf '%s\n' "Installing uv..."
  curl -LsSf https://astral.sh/uv/install.sh | sh
  # The uv installer updates shell startup files for future shells; this keeps
  # the current setup process able to run uv-dependent follow-up scripts.
  export PATH="$HOME/.local/bin:$PATH"
  command -v uv &> /dev/null || install_uv_die "uv install completed but uv is still not on PATH"
}

main() {
  ensure_uv
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
