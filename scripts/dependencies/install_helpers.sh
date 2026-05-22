#!/usr/bin/env bash
# Purpose: Shared helpers for idempotent local toolchain installers.
# Usage:   source scripts/dependencies/install_helpers.sh
# Notes:   Installers stage into a temporary sibling directory, then atomically
#          move the completed tree into place.
set -euo pipefail
shopt -s inherit_errexit

readonly KRAKEN_INSTALL_MARKER=".kraken-install-complete"
declare -ag KRAKEN_TEMP_DIRS=()

cleanup_install_temps() {
  local exit_code=$?
  local temp_dir

  for temp_dir in "${KRAKEN_TEMP_DIRS[@]:-}"; do
    if [[ -n "${temp_dir:-}" && -e "$temp_dir" ]]; then
      rm -rf -- "$temp_dir"
    fi
  done

  exit "$exit_code"
}
trap cleanup_install_temps EXIT

install_marker_path() {
  local target_dir="$1"

  printf '%s/%s\n' "$target_dir" "$KRAKEN_INSTALL_MARKER"
}

mark_install_complete() {
  local target_dir="$1"

  mkdir -p -- "$target_dir"
  : > "$(install_marker_path "$target_dir")"
}

already_installed_or_mark() {
  local install_name="$1"
  local target_dir="$2"
  local success_artifact="$3"
  local marker_path

  marker_path="$(install_marker_path "$target_dir")"

  if [[ -f "$marker_path" ]]; then
    printf '%s\n' "$install_name already installed."
    return 0
  fi

  if [[ -e "$success_artifact" ]]; then
    printf '%s\n' "$install_name already installed; marking complete."
    mark_install_complete "$target_dir"
    return 0
  fi

  if [[ -e "$target_dir" ]]; then
    printf '%s\n' "Removing partial $install_name state at $target_dir..."
    rm -rf -- "$target_dir"
  fi

  return 1
}

make_temp_dir_for_target() {
  local target_dir="$1"
  local parent_dir
  local base_name
  local temp_dir

  parent_dir="$(dirname "$target_dir")"
  base_name="$(basename "$target_dir")"

  mkdir -p -- "$parent_dir"
  temp_dir="$(mktemp -d "$parent_dir/.${base_name}.tmp.XXXXXX")"
  KRAKEN_TEMP_DIRS+=("$temp_dir")
  printf '%s\n' "$temp_dir"
}

finalize_install_dir() {
  local install_name="$1"
  local temp_dir="$2"
  local target_dir="$3"

  rm -rf -- "$target_dir"
  mv -- "$temp_dir" "$target_dir"
  mark_install_complete "$target_dir"
  printf '%s\n' "$install_name installed."
}
