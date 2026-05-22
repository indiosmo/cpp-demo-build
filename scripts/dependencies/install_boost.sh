#!/usr/bin/env bash
# Purpose: Installs Boost into the shared local toolchain prefix.
# Usage:   scripts/dependencies/install_boost.sh
# Notes:   Expects scripts/setpath.sh to have put the custom gcc on PATH.
set -euo pipefail
shopt -s inherit_errexit

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly CORES="$(nproc)"

# shellcheck source=install_helpers.sh
source "$SCRIPT_DIR/install_helpers.sh"

main() {
  local install_dir="$WORKSPACE_ROOT/boost_$BOOST_VERSION"
  local success_artifact="$install_dir/include/boost/version.hpp"
  local archive_path="$WORKSPACE_ROOT/boost_$BOOST_VERSION.tar.bz2"
  local source_dir

  if [[ -z "${KRAKEN_PATHS_SET:-}" ]]; then
    printf '%s\n' "KRAKEN_PATHS_SET is not set; source scripts/setpath.sh first." >&2
    exit 1
  fi

  printf '%s\n' "Checking boost..."
  if already_installed_or_mark "boost $BOOST_DOWNLOAD_VERSION" "$install_dir" "$success_artifact"; then
    printf '%s\n' "Boost installed."
    return
  fi

  printf '%s\n' "Building boost $BOOST_DOWNLOAD_VERSION..."
  source_dir="$(make_temp_dir_for_target "$install_dir")"

  export BOOST_ROOT="$source_dir"
  export PATH="$BOOST_ROOT/bin:$PATH"
  export LD_LIBRARY_PATH="$BOOST_ROOT/lib:${LD_LIBRARY_PATH:-}"

  curl -fSL -o "$archive_path" "https://archives.boost.io/release/$BOOST_DOWNLOAD_VERSION/source/boost_$BOOST_VERSION.tar.bz2"
  tar -xjf "$archive_path" -C "$source_dir" --strip-components=1

  (
    cd "$source_dir"
    ./bootstrap.sh --prefix="$source_dir"
    ./b2 -j"$CORES" \
      --without-python \
      toolset=gcc \
      link=static \
      variant=release \
      cxxstd=23 \
      cflags=-fPIC \
      cxxflags=-fPIC \
      boost.stacktrace.backtrace=on \
      install
  )

  finalize_install_dir "boost $BOOST_DOWNLOAD_VERSION" "$source_dir" "$install_dir"
  printf '%s\n' "Boost installed."
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
