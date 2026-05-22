#!/usr/bin/env bash
#
# Vendor management tool. Each library under submission/vendor/<name>/ is
# pinned by two text files (and optionally a third):
#
#   ORIGIN.txt    upstream git URL (one line)
#   VERSION.txt   line 1: the ref the human pinned (tag, branch, or SHA)
#                 line 2: the resolved commit SHA written by 'sync'
#   PATHS.txt     optional: one git sparse-checkout pattern per line, used
#                 when an upstream is a monorepo and we only want a subset
#
# 'sync' shallow-clones the upstream at the pinned ref into <name>/upstream/
# (with .git stripped); the per-vendor CMakeLists.txt adds the appropriate
# directory under upstream/ to its INTERFACE include path.
#
# Network is only touched by 'sync', 'check', and 'status'. Once a sync has
# run and the results are committed, the graded docker build and any cold
# clone build from bytes already in the tree (per ADR 0002).
#
# Usage:
#   scripts/vendor.sh list                    list known vendored libraries
#   scripts/vendor.sh sync   [<name>...]      clone upstream into <name>/upstream/
#   scripts/vendor.sh check  [<name>...]      re-clone at the recorded commit and
#                                             diff against <name>/upstream/
#   scripts/vendor.sh status [<name>...]      compare recorded commit to upstream HEAD
#
# Lines starting with '#' and blank lines in ORIGIN.txt / VERSION.txt /
# PATHS.txt are ignored, so the files can carry inline notes.

set -euo pipefail
shopt -s inherit_errexit

readonly script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
readonly repo_root=$(cd "${script_dir}/.." && pwd)
readonly vendor_dir="${repo_root}/submission/vendor"

log()  { printf '%s\n' "$*" >&2; }
fail() { log "vendor.sh: $*"; exit 1; }

usage() {
  sed -n '3,29p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

# Echo non-blank, non-comment lines from $1.
nonblank_lines() { awk 'NF && !/^[[:space:]]*#/' "$1"; }

# Library names that have an ORIGIN.txt under submission/vendor/, sorted.
discover_libraries() {
  [[ -d ${vendor_dir} ]] || return 0
  find "${vendor_dir}" -mindepth 2 -maxdepth 2 -name ORIGIN.txt -print \
    | while read -r path; do basename "$(dirname "${path}")"; done \
    | sort
}

read_origin() {
  local origin
  origin=$(nonblank_lines "$1/ORIGIN.txt" | head -n 1)
  [[ -n ${origin} ]] || fail "${1}/ORIGIN.txt: empty"
  printf '%s' "${origin}"
}

read_ref() {
  local ref
  ref=$(nonblank_lines "$1/VERSION.txt" | sed -n '1p')
  [[ -n ${ref} ]] || fail "${1}/VERSION.txt: missing ref on line 1"
  printf '%s' "${ref}"
}

# May be empty before the first sync.
read_commit() {
  nonblank_lines "$1/VERSION.txt" | sed -n '2p'
}

# Shallow-clone <origin>@<ref> into <dest>, honouring an optional <paths_file>
# of sparse-checkout patterns. Handles tags, branches, and bare commit SHAs
# against any host that supports uploadpack.allowReachableSHA1InWant
# (GitHub and GitLab both do).
clone_into() {
  local origin=$1 ref=$2 dest=$3 paths_file=${4:-}

  rm -rf "${dest}"
  mkdir -p "${dest}"
  git -C "${dest}" init --quiet
  git -C "${dest}" remote add origin "${origin}"

  if [[ -n ${paths_file} && -f ${paths_file} ]]; then
    git -C "${dest}" config core.sparseCheckout true
    local -a patterns=()
    while IFS= read -r pattern; do
      [[ -n ${pattern} ]] && patterns+=("${pattern}")
    done < <(nonblank_lines "${paths_file}")
    (( ${#patterns[@]} > 0 )) || fail "${paths_file}: no patterns to apply"
    git -C "${dest}" sparse-checkout init --no-cone
    git -C "${dest}" sparse-checkout set "${patterns[@]}"
  fi

  if git -C "${dest}" fetch --quiet --depth=1 origin "${ref}" 2>/dev/null; then
    git -C "${dest}" -c advice.detachedHead=false \
      checkout --quiet FETCH_HEAD
  else
    # Either the server refused a single-ref shallow fetch or the ref isn't
    # directly fetchable (e.g. a short SHA). Fall back to a full fetch.
    git -C "${dest}" fetch --quiet --tags origin
    git -C "${dest}" -c advice.detachedHead=false \
      checkout --quiet "${ref}"
  fi
}

# Overwrite VERSION.txt with the human-edited ref on line 1 and the
# resolved commit SHA on line 2.
write_version() {
  local dir=$1 ref=$2 commit=$3
  cat > "${dir}/VERSION.txt" <<EOF
${ref}
${commit}
EOF
}

sync_one() {
  local name=$1
  local dir="${vendor_dir}/${name}"
  [[ -f ${dir}/ORIGIN.txt && -f ${dir}/VERSION.txt ]] \
    || fail "${name}: missing ORIGIN.txt or VERSION.txt"

  local origin ref
  origin=$(read_origin "${dir}")
  ref=$(read_ref "${dir}")

  local scratch
  scratch=$(mktemp -d -t vendor-sync.XXXXXX)
  # shellcheck disable=SC2064
  trap "rm -rf '${scratch}'" RETURN

  log "==> ${name}: cloning ${origin}@${ref}"
  clone_into "${origin}" "${ref}" "${scratch}/upstream" "${dir}/PATHS.txt"

  local commit
  commit=$(git -C "${scratch}/upstream" rev-parse HEAD)

  # The committed bytes are the upstream tree minus .git. We rely on the
  # parent repo's git history (and code review) as the integrity check on
  # those bytes; 'vendor.sh check' verifies them against upstream.
  rm -rf "${scratch}/upstream/.git"

  rm -rf "${dir}/upstream"
  mv "${scratch}/upstream" "${dir}/upstream"

  write_version "${dir}" "${ref}" "${commit}"
  log "==> ${name}: synced ${commit}"
}

check_one() {
  local name=$1
  local dir="${vendor_dir}/${name}"

  [[ -d ${dir}/upstream ]] || { log "${name}: not synced (no upstream/)"; return 1; }

  local origin commit
  origin=$(read_origin "${dir}")
  commit=$(read_commit "${dir}")
  [[ -n ${commit} ]] || { log "${name}: VERSION.txt has no resolved commit; run sync"; return 1; }

  local scratch
  scratch=$(mktemp -d -t vendor-check.XXXXXX)
  # shellcheck disable=SC2064
  trap "rm -rf '${scratch}'" RETURN

  log "==> ${name}: re-cloning at ${commit}"
  clone_into "${origin}" "${commit}" "${scratch}/upstream" "${dir}/PATHS.txt"
  rm -rf "${scratch}/upstream/.git"

  if diff -r --brief "${scratch}/upstream" "${dir}/upstream" >/dev/null 2>&1; then
    log "==> ${name}: ok"
    return 0
  fi

  log "${name}: vendored bytes differ from upstream@${commit}:"
  diff -r --brief "${scratch}/upstream" "${dir}/upstream" >&2 || true
  log "==> ${name}: FAIL"
  return 1
}

status_one() {
  local name=$1
  local dir="${vendor_dir}/${name}"
  local origin ref pinned_commit upstream_commit mark
  origin=$(read_origin "${dir}")
  ref=$(read_ref "${dir}")
  pinned_commit=$(read_commit "${dir}")
  upstream_commit=$(git ls-remote "${origin}" "${ref}" \
    | awk 'NR==1 {print $1}')
  [[ -n ${upstream_commit} ]] || upstream_commit="(no match for ref)"
  if [[ ${pinned_commit} == "${upstream_commit}" ]]; then
    mark="up-to-date"
  else
    mark="behind"
  fi
  printf '%-24s ref=%-12s pinned=%s upstream=%s [%s]\n' \
    "${name}" "${ref}" "${pinned_commit:-(none)}" "${upstream_commit}" "${mark}"
}

list_one() {
  local name=$1
  local dir="${vendor_dir}/${name}"
  local origin ref pinned_commit
  origin=$(read_origin "${dir}")
  ref=$(read_ref "${dir}")
  pinned_commit=$(read_commit "${dir}")
  printf '%-24s %s@%s  commit=%s\n' \
    "${name}" "${origin}" "${ref}" "${pinned_commit:-(not yet synced)}"
}

resolve_names() {
  local -a names=("$@")
  if (( ${#names[@]} == 0 )); then
    mapfile -t names < <(discover_libraries)
  fi
  (( ${#names[@]} > 0 )) || fail "no vendored libraries found under ${vendor_dir}"
  printf '%s\n' "${names[@]}"
}

cmd_list() {
  local -a names
  mapfile -t names < <(resolve_names "$@")
  for name in "${names[@]}"; do list_one "${name}"; done
}

cmd_sync() {
  local -a names
  mapfile -t names < <(resolve_names "$@")
  for name in "${names[@]}"; do sync_one "${name}"; done
}

cmd_check() {
  local -a names
  mapfile -t names < <(resolve_names "$@")
  local failed=0
  for name in "${names[@]}"; do
    check_one "${name}" || failed=$((failed + 1))
  done
  (( failed == 0 )) || { log "check: ${failed} failure(s)"; return 1; }
}

cmd_status() {
  local -a names
  mapfile -t names < <(resolve_names "$@")
  for name in "${names[@]}"; do status_one "${name}"; done
}

main() {
  local cmd=${1:-}
  case ${cmd} in
    list|ls)         shift; cmd_list "$@" ;;
    sync)            shift; cmd_sync "$@" ;;
    check|verify)    shift; cmd_check "$@" ;;
    status)          shift; cmd_status "$@" ;;
    -h|--help|help)  usage ;;
    "")              usage; exit 2 ;;
    *)               log "unknown command: ${cmd}"; usage; exit 2 ;;
  esac
}

main "$@"
