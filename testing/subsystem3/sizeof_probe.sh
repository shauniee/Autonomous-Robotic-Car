#!/bin/sh
# Cortex-M0+ sizeof probe. Run from anywhere:
#   testing/subsystem3/sizeof_probe.sh [optional/path/to/sizeof_probe.c]
# Default remains the S3 probe for existing callers; other subsystems may
# supply their own source without copying the build flags.
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
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
    -c "$probe_source" -o "$obj"
arm-none-eabi-nm -S -t d --size-sort "$obj" |
    awk '$4 ~ /^probe_/ { sub(/^probe_/, "", $4); printf "%d %s\\n", $2, $4 }'
