#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_dir=$(cd -- "$script_dir/.." && pwd)
default_build_dir="$repo_dir/build-uefi"
if [[ $# -gt 2 ]]; then
    echo "Usage: $0 [ISO_IMAGE] [--window]" >&2
    exit 2
fi
image=${1:-"$default_build_dir/ER-DISKOS.iso"}
mode=${2:-}
build_dir=$(dirname -- "$image")
qemu_bin=${QEMU_BIN:-qemu-system-x86_64}
ovmf_code=${OVMF_CODE:-/usr/share/OVMF/OVMF_CODE_4M.fd}
ovmf_vars_template=${OVMF_VARS_TEMPLATE:-/usr/share/OVMF/OVMF_VARS_4M.fd}
ovmf_vars=${OVMF_VARS_FILE:-"$build_dir/OVMF_VARS_ISO.fd"}

case "$mode" in
    "" )
        qemu_display_args=(-display none -serial stdio)
        ;;
    --window)
        qemu_display_args=(-display gtk -serial none)
        ;;
    *)
        echo "Usage: $0 [ESP_IMAGE] [--window]" >&2
        exit 2
        ;;
esac

if [[ ! -f "$image" ]]; then
    echo "UEFI ISO not found: $image" >&2
    echo "Build it with: cmake --build <build-dir> --target uefi_iso" >&2
    exit 1
fi
if ! command -v "$qemu_bin" >/dev/null 2>&1; then
    echo "QEMU executable not found: $qemu_bin" >&2
    exit 1
fi
if [[ ! -f "$ovmf_code" || ! -f "$ovmf_vars_template" ]]; then
    echo "OVMF firmware files not found. Set OVMF_CODE and OVMF_VARS_TEMPLATE." >&2
    exit 1
fi

mkdir -p "$build_dir" "$(dirname -- "$ovmf_vars")"
if [[ ! -f "$ovmf_vars" ]]; then
    cp "$ovmf_vars_template" "$ovmf_vars"
fi

exec "$qemu_bin" \
    -machine q35 \
    -m 256M \
    -drive "if=pflash,format=raw,readonly=on,file=$ovmf_code" \
    -drive "if=pflash,format=raw,file=$ovmf_vars" \
    -cdrom "$image" \
    -boot order=d \
    "${qemu_display_args[@]}" \
    -monitor none \
    -no-reboot \
    -net none
