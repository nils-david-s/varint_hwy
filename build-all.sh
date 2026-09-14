#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p "${ROOT_DIR}/out"

build() {
    local name="$1"
    local tc="$2"
    local builddir="${ROOT_DIR}/out/build-${name}"
    cmake \
        -S "${ROOT_DIR}" \
        -B "${builddir}" \
        -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/toolchains/${tc}" \
        -DOPT_LEVEL="O3"
    cmake \
        --build "${builddir}" \
        -- -j$(nproc)
}

build riscv toolchain-riscv.cmake
build x86_64 toolchain-x86_64.cmake
build arm toolchain-arm.cmake
