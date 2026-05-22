#!/usr/bin/env bash
# Purpose: Adds the shared C++ toolchain paths to the current shell.
# Usage:   source scripts/setpath.sh
# Notes:   Defaults to ~/cpp_workspace for the shared GCC, CMake, Boost,
#          ccache, mold, binutils, and lcov installs.
set -euo pipefail
shopt -s inherit_errexit

readonly SETPATH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck source=versions.sh
source "$SETPATH_DIR/versions.sh"

export WORKSPACE_ROOT="${WORKSPACE_ROOT:-$HOME/cpp_workspace}"
export LLVM_ROOT="${LLVM_ROOT:-/usr/lib/llvm-$LLVM_VERSION}"
export GCC_ROOT="${GCC_ROOT:-$WORKSPACE_ROOT/gcc-$GCC_VERSION}"
export BINUTILS_ROOT="${BINUTILS_ROOT:-$WORKSPACE_ROOT/binutils-$BINUTILS_VERSION}"
export LIBBACKTRACE_ROOT="${LIBBACKTRACE_ROOT:-$WORKSPACE_ROOT/libbacktrace-$GCC_VERSION}"
export CMAKE_ROOT="${CMAKE_ROOT:-$WORKSPACE_ROOT/cmake-$CMAKE_VERSION.$CMAKE_BUILD_VERSION}"
export CCACHE_ROOT="${CCACHE_ROOT:-$WORKSPACE_ROOT/ccache-$CCACHE_VERSION}"
export MOLD_ROOT="${MOLD_ROOT:-$WORKSPACE_ROOT/mold-$MOLD_VERSION}"
export LCOV_ROOT="${LCOV_ROOT:-$WORKSPACE_ROOT/lcov-$LCOV_VERSION}"
export BOOST_ROOT="${BOOST_ROOT:-$WORKSPACE_ROOT/boost_$BOOST_VERSION}"

export PATH="$LLVM_ROOT/bin:$MOLD_ROOT/bin:$GCC_ROOT/bin:$BINUTILS_ROOT/bin:$CCACHE_ROOT/bin:$CMAKE_ROOT/bin:$LCOV_ROOT/bin:$PATH"
export LD_LIBRARY_PATH="$GCC_ROOT/lib64:$BINUTILS_ROOT/lib:${LD_LIBRARY_PATH:-}"
export CPATH="$LIBBACKTRACE_ROOT/include:${CPATH:-}"
export LIBRARY_PATH="$LIBBACKTRACE_ROOT/lib:${LIBRARY_PATH:-}"

export KRAKEN_PATHS_SET=1
