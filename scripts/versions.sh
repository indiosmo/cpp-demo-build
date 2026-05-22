#!/usr/bin/env bash
# Purpose: Defines the local toolchain versions used by setup and CMake presets.
# Usage:   source scripts/versions.sh
# Notes:   Keep these values aligned across setup scripts and CMake presets.
set -euo pipefail
shopt -s inherit_errexit

export GCC_VERSION=16.1.0
export LLVM_VERSION=23
export BINUTILS_VERSION=2.46.0
export CMAKE_VERSION=4.3
export CMAKE_BUILD_VERSION=2
export CCACHE_VERSION=4.13.5
export MOLD_VERSION=2.41.0
export LCOV_VERSION=2.3.2

BOOST_MAJOR_VERSION=1
BOOST_MINOR_VERSION=91
BOOST_PATCH_VERSION=0
export BOOST_VERSION=${BOOST_MAJOR_VERSION}_${BOOST_MINOR_VERSION}_${BOOST_PATCH_VERSION}
export BOOST_DOWNLOAD_VERSION=${BOOST_MAJOR_VERSION}.${BOOST_MINOR_VERSION}.${BOOST_PATCH_VERSION}
