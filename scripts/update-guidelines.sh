#!/usr/bin/env bash
# Purpose: Updates the cpp-guidelines submodule to the latest origin default branch commit.
# Usage:   update-guidelines.sh
# Notes:   Stages only the submodule pointer bump in the superproject.
set -euo pipefail
shopt -s inherit_errexit

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
readonly SUBMODULE_PATH="docs/cpp-guidelines"

die() {
  printf '%s\n' "$*" >&2
  exit 1
}

log() {
  printf '%s\n' "$*" >&2
}

ensure_submodule_initialized() {
  if [[ ! -f "$SUBMODULE_PATH/.git" && ! -d "$SUBMODULE_PATH/.git" ]]; then
    log "Initializing $SUBMODULE_PATH..."
    git submodule update --init --recursive "$SUBMODULE_PATH"
  fi
}

ensure_clean_superproject_pointer() {
  if ! git diff --quiet -- "$SUBMODULE_PATH" || ! git diff --cached --quiet -- "$SUBMODULE_PATH"; then
    die "$SUBMODULE_PATH already has staged or unstaged pointer changes. Commit or reset them before updating."
  fi
}

ensure_clean_submodule() {
  local status_output
  status_output="$(git -C "$SUBMODULE_PATH" status --porcelain)"

  if [[ -n "$status_output" ]]; then
    die "$SUBMODULE_PATH has local changes. Commit or stash them before updating."
  fi
}

default_branch() {
  local remote_head
  if ! remote_head="$(git -C "$SUBMODULE_PATH" symbolic-ref --quiet --short refs/remotes/origin/HEAD)"; then
    die "Unable to determine origin default branch for $SUBMODULE_PATH."
  fi

  printf '%s\n' "${remote_head#origin/}"
}

fetch_origin() {
  log "Fetching latest cpp-guidelines..."
  git -C "$SUBMODULE_PATH" fetch --prune origin
}

checkout_origin_branch() {
  local branch="$1"
  local remote_ref="origin/$branch"

  log "Checking out $remote_ref..."
  git -C "$SUBMODULE_PATH" checkout --detach "$remote_ref"
}

stage_pointer_bump() {
  local new_sha

  if git diff --quiet -- "$SUBMODULE_PATH"; then
    log "cpp-guidelines already at the latest commit."
    return 0
  fi

  git add "$SUBMODULE_PATH"
  new_sha="$(git -C "$SUBMODULE_PATH" rev-parse --short HEAD)"

  printf '\n'
  printf 'Staged cpp-guidelines pointer bump to %s.\n' "$new_sha"
  printf 'Review with: git diff --cached -- %s\n' "$SUBMODULE_PATH"
  printf "Commit with: git commit -m 'Bump cpp-guidelines to %s'\n" "$new_sha"
}

main() {
  local branch

  cd "$REPO_ROOT"
  ensure_submodule_initialized
  ensure_clean_superproject_pointer
  ensure_clean_submodule

  fetch_origin
  branch="$(default_branch)"
  checkout_origin_branch "$branch"
  stage_pointer_bump
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
