#!/usr/bin/env bash
# Purpose: Installs the system and local dependencies for this repository.
# Usage:   scripts/install_dependencies.sh
# Notes:   Installs OS packages with sudo, then builds the shared GCC 16
#          toolchain and Boost under WORKSPACE_ROOT.
set -euo pipefail
shopt -s inherit_errexit

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# shellcheck source=versions.sh
source "$SCRIPT_DIR/versions.sh"
# shellcheck source=setpath.sh
source "$SCRIPT_DIR/setpath.sh"
# shellcheck source=/etc/os-release
source /etc/os-release

readonly RELEASE_NAME="${VERSION_CODENAME:?VERSION_CODENAME is not set}"
readonly CLANG_LIST="/etc/apt/sources.list.d/clang.list"
readonly LLVM_KEYRING="/usr/share/keyrings/llvm-archive-keyring.gpg"
readonly LLVM_REPO="deb [signed-by=$LLVM_KEYRING] http://apt.llvm.org/${RELEASE_NAME}/ llvm-toolchain-${RELEASE_NAME} main"
readonly LLVM_SRC_REPO="deb-src [signed-by=$LLVM_KEYRING] http://apt.llvm.org/${RELEASE_NAME}/ llvm-toolchain-${RELEASE_NAME} main"

install_llvm_keyring() {
  local temp_root
  local tmp_keyring

  if sudo test -s "$LLVM_KEYRING"; then
    return
  fi

  printf '%s\n' "Installing LLVM signing key..."
  temp_root="${TMPDIR:-$WORKSPACE_ROOT/tmp}"
  mkdir -p -- "$temp_root"
  tmp_keyring="$(mktemp "$temp_root/kraken-llvm-keyring.XXXXXX")"
  curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key | gpg --dearmor > "$tmp_keyring"
  sudo install -Dm644 "$tmp_keyring" "$LLVM_KEYRING"
  rm -f -- "$tmp_keyring"
}

ensure_repo_prerequisites() {
  local -a missing_packages=()

  if ! command -v curl > /dev/null 2>&1; then
    missing_packages+=(curl)
  fi

  if ! command -v gpg > /dev/null 2>&1; then
    missing_packages+=(gnupg)
  fi

  if (( ${#missing_packages[@]} == 0 )); then
    return
  fi

  printf '%s\n' "Installing repository prerequisites..."
  sudo apt -y install ca-certificates "${missing_packages[@]}"
}

write_llvm_repo_file() {
  if sudo test -f "$CLANG_LIST" \
    && sudo grep -Fqx "$LLVM_REPO" "$CLANG_LIST" \
    && sudo grep -Fqx "$LLVM_SRC_REPO" "$CLANG_LIST"; then
    return
  fi

  printf '%s\n' "Writing LLVM repository definition..."
  printf '%s\n%s\n' "$LLVM_REPO" "$LLVM_SRC_REPO" | sudo tee "$CLANG_LIST" > /dev/null
}

install_system_packages() {
  printf '%s\n' "Installing system packages..."
  sudo apt update
  sudo apt -y install \
    build-essential \
    ca-certificates \
    curl \
    flex \
    gpg \
    tar \
    gzip \
    bzip2 \
    xz-utils \
    pkg-config \
    ninja-build \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    texinfo \
    libcurl4-openssl-dev \
    clang-"$LLVM_VERSION" \
    clangd-"$LLVM_VERSION" \
    clang-format-"$LLVM_VERSION" \
    clang-tidy-"$LLVM_VERSION" \
    clang-tools-"$LLVM_VERSION" \
    libclang1-"$LLVM_VERSION" \
    lld-"$LLVM_VERSION" \
    libfmt-dev \
    libspdlog-dev \
    catch2 \
    libbenchmark-dev
}

main() {
  ensure_repo_prerequisites
  install_llvm_keyring
  write_llvm_repo_file
  install_system_packages

  "$SCRIPT_DIR/dependencies/install_build_tools.sh"

  # Boost is built after setpath so its bootstrap finds the custom GCC.
  # shellcheck source=setpath.sh
  source "$SCRIPT_DIR/setpath.sh"
  "$SCRIPT_DIR/dependencies/install_boost.sh"

  printf '%s\n' "All dependencies installed."
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
  main "$@"
fi
