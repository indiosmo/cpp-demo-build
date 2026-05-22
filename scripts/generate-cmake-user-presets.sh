#!/usr/bin/env bash
# Purpose: Generates CMakeUserPresets.json with resolved local toolchain paths.
# Usage:   scripts/generate-cmake-user-presets.sh
# Notes:   Useful for editors that launch CMake without sourcing setenv.sh.
set -euo pipefail
shopt -s inherit_errexit

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
readonly OUTPUT_FILE="$REPO_ROOT/CMakeUserPresets.json"

# shellcheck source=versions.sh
source "$SCRIPT_DIR/versions.sh"
# shellcheck source=setpath.sh
source "$SCRIPT_DIR/setpath.sh"

expand_path() {
  local path="$1"

  printf '%s\n' "${path/#\~/$HOME}"
}

main() {
  local workspace_root
  local llvm_root
  local gcc_root
  local binutils_root
  local libbacktrace_root
  local cmake_root
  local ccache_root
  local mold_root
  local lcov_root
  local boost_root

  workspace_root="$(expand_path "$WORKSPACE_ROOT")"
  llvm_root="$(expand_path "$LLVM_ROOT")"
  gcc_root="$(expand_path "$GCC_ROOT")"
  binutils_root="$(expand_path "$BINUTILS_ROOT")"
  libbacktrace_root="$(expand_path "$LIBBACKTRACE_ROOT")"
  cmake_root="$(expand_path "$CMAKE_ROOT")"
  ccache_root="$(expand_path "$CCACHE_ROOT")"
  mold_root="$(expand_path "$MOLD_ROOT")"
  lcov_root="$(expand_path "$LCOV_ROOT")"
  boost_root="$(expand_path "$BOOST_ROOT")"

  printf '%s\n' \
    "{" \
    "  \"version\": 6," \
    "  \"configurePresets\": [" \
    "    {" \
    "      \"name\": \"vscode-debug\"," \
    "      \"inherits\": \"debug\"," \
    "      \"displayName\": \"Debug (local GCC 16 toolchain)\"," \
    "      \"environment\": {" \
    "        \"WORKSPACE_ROOT\": \"$workspace_root\"," \
    "        \"LLVM_VERSION\": \"$LLVM_VERSION\"," \
    "        \"LLVM_ROOT\": \"$llvm_root\"," \
    "        \"GCC_VERSION\": \"$GCC_VERSION\"," \
    "        \"GCC_ROOT\": \"$gcc_root\"," \
    "        \"BINUTILS_VERSION\": \"$BINUTILS_VERSION\"," \
    "        \"BINUTILS_ROOT\": \"$binutils_root\"," \
    "        \"LIBBACKTRACE_ROOT\": \"$libbacktrace_root\"," \
    "        \"CMAKE_ROOT\": \"$cmake_root\"," \
    "        \"CCACHE_ROOT\": \"$ccache_root\"," \
    "        \"MOLD_ROOT\": \"$mold_root\"," \
    "        \"LCOV_ROOT\": \"$lcov_root\"," \
    "        \"BOOST_ROOT\": \"$boost_root\"," \
    "        \"PATH\": \"$mold_root/bin:$gcc_root/bin:$binutils_root/bin:$ccache_root/bin:$cmake_root/bin:$lcov_root/bin:\$penv{PATH}\"," \
    "        \"LD_LIBRARY_PATH\": \"$gcc_root/lib64:$binutils_root/lib:\$penv{LD_LIBRARY_PATH}\"," \
    "        \"CPATH\": \"$libbacktrace_root/include:\$penv{CPATH}\"," \
    "        \"LIBRARY_PATH\": \"$libbacktrace_root/lib:\$penv{LIBRARY_PATH}\"" \
    "      }" \
    "    }" \
    "  ]" \
    "}" > "$OUTPUT_FILE"

  printf '%s\n' "Generated $OUTPUT_FILE"
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
