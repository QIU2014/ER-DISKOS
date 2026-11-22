#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 5 ]]; then
    echo "Usage: $0 IMAGE SIZE_MIB BOOTX64_EFI JAVA_CLASSES_DIR APP_CLASSES_DIR" >&2
    exit 2
fi

image=$1
size_mib=$2
efi_app=$3
classes_dir=$4
app_classes_dir=$5

if [[ ! "$size_mib" =~ ^[1-9][0-9]*$ ]]; then
    echo "Invalid ESP size in MiB: $size_mib" >&2
    exit 2
fi
for input in "$efi_app" "$classes_dir/sys.properties" "$app_classes_dir/com/eric/Main.class"; do
    if [[ ! -f "$input" ]]; then
        echo "Required ESP input not found: $input" >&2
        exit 1
    fi
done

for tool in truncate mkfs.vfat mcopy mmd mdir; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Required image tool not found: $tool" >&2
        exit 1
    fi
done

mkdir -p "$(dirname "$image")"
rm -f "$image"
truncate -s "${size_mib}M" "$image"
mkfs.vfat -F 32 -n ERDISKOS "$image" >/dev/null

mmd -i "$image" ::/EFI ::/EFI/BOOT ::/jvm
mcopy -i "$image" "$efi_app" ::/EFI/BOOT/BOOTX64.EFI

for entry in "$classes_dir"/*; do
    if [[ -e "$entry" ]]; then
        mcopy -s -i "$image" "$entry" ::/jvm/
    fi
done

for entry in "$app_classes_dir"/*; do
    if [[ -e "$entry" ]]; then
        mcopy -s -o -i "$image" "$entry" ::/jvm/
    fi
done

echo "Created bootable ESP image: $image"
