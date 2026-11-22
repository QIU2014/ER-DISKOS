#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 6 ]]; then
    echo "Usage: $0 ISO SIZE_MIB BOOTX64_EFI JAVA_CLASSES_DIR APP_CLASSES_DIR MKISOFS" >&2
    exit 2
fi

iso=$1
size_mib=$2
efi_app=$3
classes_dir=$4
app_classes_dir=$5
mkisofs=$6
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
staging_dir=$(mktemp -d)
trap 'rm -rf "$staging_dir"' EXIT

esp_image="$staging_dir/ER-DISKOS-ESP.img"
bash "$script_dir/create-uefi-esp.sh" \
    "$esp_image" "$size_mib" "$efi_app" "$classes_dir" "$app_classes_dir"

mkdir -p "$(dirname "$iso")"
rm -f "$iso"
"$mkisofs" -quiet -R -J -V ERDISKOS -o "$iso" \
    -eltorito-alt-boot -e ER-DISKOS-ESP.img -no-emul-boot "$staging_dir"

echo "Created bootable UEFI ISO: $iso"
