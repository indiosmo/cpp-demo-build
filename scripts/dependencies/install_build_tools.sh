#!/usr/bin/env bash
# Purpose: Installs the shared compiler and build-tool chain from source.
# Usage:   scripts/dependencies/install_build_tools.sh
# Notes:   Uses WORKSPACE_ROOT from scripts/setpath.sh; defaults to
#          ~/cpp_workspace.
set -euo pipefail
shopt -s inherit_errexit

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly CORES="$(nproc)"

# shellcheck source=install_helpers.sh
source "$SCRIPT_DIR/install_helpers.sh"

require_workspace_root() {
  if [[ -z "${WORKSPACE_ROOT:-}" ]]; then
    printf '%s\n' "WORKSPACE_ROOT is not set" >&2
    exit 1
  fi

  mkdir -p -- "$WORKSPACE_ROOT"
}

install_gcc() {
  local install_dir="$WORKSPACE_ROOT/gcc-$GCC_VERSION"
  local success_artifact="$install_dir/bin/g++"
  local archive_path="$WORKSPACE_ROOT/gcc-$GCC_VERSION.tar.gz"
  local source_dir
  local build_dir

  printf '%s\n' "Checking gcc..."
  if already_installed_or_mark "gcc $GCC_VERSION" "$install_dir" "$success_artifact"; then
    return
  fi

  printf '%s\n' "Building gcc $GCC_VERSION..."
  source_dir="$(make_temp_dir_for_target "$install_dir")"
  build_dir="$(make_temp_dir_for_target "$WORKSPACE_ROOT/gcc-$GCC_VERSION-build")"

  curl -fSL -o "$archive_path" "https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz"
  tar -xzf "$archive_path" -C "$source_dir" --strip-components=1

  (
    cd "$source_dir"
    ./contrib/download_prerequisites
  )

  (
    cd "$build_dir"
    "$source_dir/configure" --prefix="$source_dir" --disable-multilib
    make -j"$CORES"
    make install
  )

  rm -rf -- "$build_dir"
  finalize_install_dir "gcc $GCC_VERSION" "$source_dir" "$install_dir"
}

install_libbacktrace() {
  local install_dir="$WORKSPACE_ROOT/libbacktrace-$GCC_VERSION"
  local success_artifact="$install_dir/lib/libbacktrace.a"
  local libbacktrace_source="$WORKSPACE_ROOT/gcc-$GCC_VERSION/libbacktrace"
  local install_dir_tmp
  local build_dir

  printf '%s\n' "Checking libbacktrace..."
  if already_installed_or_mark "libbacktrace $GCC_VERSION" "$install_dir" "$success_artifact"; then
    return
  fi

  if [[ ! -x "$libbacktrace_source/configure" ]]; then
    printf '%s\n' "Missing libbacktrace source at $libbacktrace_source; install gcc first." >&2
    exit 1
  fi

  printf '%s\n' "Building libbacktrace from gcc $GCC_VERSION source..."
  install_dir_tmp="$(make_temp_dir_for_target "$install_dir")"
  build_dir="$(make_temp_dir_for_target "$WORKSPACE_ROOT/libbacktrace-$GCC_VERSION-build")"

  (
    cd "$build_dir"
    "$libbacktrace_source/configure" \
      --prefix="$install_dir_tmp" \
      --with-pic
    make -j"$CORES"

    # gcc keeps libbacktrace as a no-install library, so install its public
    # headers and static archive into the staged prefix explicitly.
    mkdir -p "$install_dir_tmp/include" "$install_dir_tmp/lib"
    cp "$libbacktrace_source/backtrace.h" "$install_dir_tmp/include/"
    cp backtrace-supported.h "$install_dir_tmp/include/"
    cp .libs/libbacktrace.a "$install_dir_tmp/lib/"
  )

  rm -rf -- "$build_dir"
  finalize_install_dir "libbacktrace $GCC_VERSION" "$install_dir_tmp" "$install_dir"
}

install_binutils() {
  local install_dir="$WORKSPACE_ROOT/binutils-$BINUTILS_VERSION"
  local success_artifact="$install_dir/bin/ld"
  local archive_path="$WORKSPACE_ROOT/binutils-$BINUTILS_VERSION.tar.gz"
  local install_dir_tmp
  local source_dir
  local build_dir

  printf '%s\n' "Checking binutils..."
  if already_installed_or_mark "binutils $BINUTILS_VERSION" "$install_dir" "$success_artifact"; then
    return
  fi

  printf '%s\n' "Building binutils $BINUTILS_VERSION..."
  install_dir_tmp="$(make_temp_dir_for_target "$install_dir")"
  source_dir="$(make_temp_dir_for_target "$WORKSPACE_ROOT/binutils-$BINUTILS_VERSION-source")"
  build_dir="$(make_temp_dir_for_target "$WORKSPACE_ROOT/binutils-$BINUTILS_VERSION-build")"

  curl -fSL -o "$archive_path" "https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz"
  tar -xzf "$archive_path" -C "$source_dir" --strip-components=1

  (
    cd "$build_dir"
    "$source_dir/configure" --prefix="$install_dir_tmp"
    make -j"$CORES"
    make install
  )

  rm -rf -- "$build_dir" "$source_dir"
  finalize_install_dir "binutils $BINUTILS_VERSION" "$install_dir_tmp" "$install_dir"
}

install_cmake() {
  local install_dir="$WORKSPACE_ROOT/cmake-$CMAKE_VERSION.$CMAKE_BUILD_VERSION"
  local success_artifact="$install_dir/bin/cmake"
  local archive_path="$WORKSPACE_ROOT/cmake-$CMAKE_VERSION.$CMAKE_BUILD_VERSION.tar.gz"
  local source_dir

  printf '%s\n' "Checking cmake..."
  if already_installed_or_mark "cmake $CMAKE_VERSION.$CMAKE_BUILD_VERSION" "$install_dir" "$success_artifact"; then
    return
  fi

  printf '%s\n' "Building cmake $CMAKE_VERSION.$CMAKE_BUILD_VERSION..."
  source_dir="$(make_temp_dir_for_target "$install_dir")"

  curl -fSL -o "$archive_path" "https://cmake.org/files/v$CMAKE_VERSION/cmake-$CMAKE_VERSION.$CMAKE_BUILD_VERSION.tar.gz"
  tar -xzf "$archive_path" -C "$source_dir" --strip-components=1

  (
    cd "$source_dir"
    ./bootstrap --prefix="$source_dir" --parallel="$CORES" --system-curl
    make -j"$CORES"
    make install
  )

  finalize_install_dir "cmake $CMAKE_VERSION.$CMAKE_BUILD_VERSION" "$source_dir" "$install_dir"
}

install_ccache() {
  local install_dir="$WORKSPACE_ROOT/ccache-$CCACHE_VERSION"
  local success_artifact="$install_dir/bin/ccache"
  local archive_path="$WORKSPACE_ROOT/ccache-$CCACHE_VERSION.tar.gz"
  local source_dir

  printf '%s\n' "Checking ccache..."
  if already_installed_or_mark "ccache $CCACHE_VERSION" "$install_dir" "$success_artifact"; then
    return
  fi

  printf '%s\n' "Building ccache $CCACHE_VERSION..."
  source_dir="$(make_temp_dir_for_target "$install_dir")"

  curl -fSL -o "$archive_path" "https://github.com/ccache/ccache/releases/download/v$CCACHE_VERSION/ccache-$CCACHE_VERSION.tar.gz"
  tar -xzf "$archive_path" -C "$source_dir" --strip-components=1

  cmake -S "$source_dir" -B "$source_dir/_build" -DCMAKE_INSTALL_PREFIX="$source_dir" -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Release -DZSTD_FROM_INTERNET=ON -DENABLE_TESTING=OFF
  cmake --build "$source_dir/_build" --target install -- -j"$CORES"

  finalize_install_dir "ccache $CCACHE_VERSION" "$source_dir" "$install_dir"
}

install_mold() {
  local install_dir="$WORKSPACE_ROOT/mold-$MOLD_VERSION"
  local success_artifact="$install_dir/bin/mold"
  local archive_path="$WORKSPACE_ROOT/mold-$MOLD_VERSION-x86_64-linux.tar.gz"
  local install_dir_tmp

  printf '%s\n' "Checking mold..."
  if already_installed_or_mark "mold $MOLD_VERSION" "$install_dir" "$success_artifact"; then
    return
  fi

  printf '%s\n' "Installing mold $MOLD_VERSION..."
  install_dir_tmp="$(make_temp_dir_for_target "$install_dir")"

  curl -fSL -o "$archive_path" "https://github.com/rui314/mold/releases/download/v$MOLD_VERSION/mold-$MOLD_VERSION-x86_64-linux.tar.gz"
  tar -xzf "$archive_path" -C "$install_dir_tmp" --strip-components=1

  finalize_install_dir "mold $MOLD_VERSION" "$install_dir_tmp" "$install_dir"
}

install_lcov() {
  local install_dir="$WORKSPACE_ROOT/lcov-$LCOV_VERSION"
  local success_artifact="$install_dir/bin/lcov"
  local archive_path="$WORKSPACE_ROOT/lcov-$LCOV_VERSION.tar.gz"
  local install_dir_tmp

  printf '%s\n' "Checking lcov..."
  if already_installed_or_mark "lcov $LCOV_VERSION" "$install_dir" "$success_artifact"; then
    return
  fi

  printf '%s\n' "Installing lcov $LCOV_VERSION..."
  install_dir_tmp="$(make_temp_dir_for_target "$install_dir")"

  curl -fSL -o "$archive_path" "https://github.com/linux-test-project/lcov/releases/download/v$LCOV_VERSION/lcov-$LCOV_VERSION.tar.gz"
  tar -xzf "$archive_path" -C "$install_dir_tmp" --strip-components=1

  finalize_install_dir "lcov $LCOV_VERSION" "$install_dir_tmp" "$install_dir"
}

main() {
  require_workspace_root
  install_gcc
  install_libbacktrace
  install_binutils
  install_cmake
  install_ccache
  install_mold
  install_lcov
  printf '%s\n' "Build tools installed."
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
