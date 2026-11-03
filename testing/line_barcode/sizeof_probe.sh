#!/bin/sh
# Cortex-M0+ sizeof probe. Run from anywhere:
#   testing/line_barcode/sizeof_probe.sh [optional/path/to/sizeof_probe.c]
# Default remains the line_barcode probe for existing callers; other subsystems may
# supply their own source without copying the build flags.
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
probe_source=${1:-"$script_dir/sizeof_probe.c"}
if [ ! -f "$probe_source" ]; then
    echo "probe source not found: $probe_source" >&2
    exit 1
fi
obj=$(mktemp)
trap 'rm -f "$obj"' EXIT HUP INT TERM

# Layout-relevant target flags from build_make/pico_rp2040.mk CFLAGS; -fcommon
# only keeps the probe arrays as sized symbols for nm.
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -ffreestanding -std=gnu11 -mfloat-abi=soft -fcommon \
    -D_PICO_RP2040_ -DCNF_SMP=1 -I"$repo_root/include" -I"$repo_root/config" \
    -c "$probe_source" -o "$obj"
arm-none-eabi-nm -S -t d --size-sort "$obj" |
    awk '$4 ~ /^probe_/ { sub(/^probe_/, "", $4); printf "%d %s\n", $2, $4 }'
