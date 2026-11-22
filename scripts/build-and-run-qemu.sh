#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_dir=$(cd -- "$script_dir/.." && pwd)

if [[ $# -gt 1 ]]; then
    echo "Usage: $0 [BUILD_DIR]" >&2
    exit 2
fi

build_dir=${1:-${UEFI_BUILD_DIR:-"$repo_dir/build-uefi"}}
toolchain_file=${UEFI_TOOLCHAIN_FILE:-"$repo_dir/UEFI-x86_64-toolchain.cmake"}
image=${UEFI_ISO_IMAGE:-"$build_dir/ER-DISKOS.iso"}
if [[ "$image" != /* ]]; then
    image="$PWD/$image"
fi

cmake -S "$repo_dir" -B "$build_dir" \
    -DCMAKE_TOOLCHAIN_FILE="$toolchain_file" \
    -DUEFI_ISO_IMAGE="$image"
cmake --build "$build_dir" --target uefi_iso \
    --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"

exec bash "$script_dir/run-qemu.sh" "$image" --window
