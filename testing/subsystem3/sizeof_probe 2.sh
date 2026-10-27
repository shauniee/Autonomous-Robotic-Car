#!/bin/sh
# S3 sizeof probe for Cortex-M0+. Run from anywhere: testing/subsystem3/sizeof_probe.sh
# Prints "<bytes> <probe>" per line; compile only, no firmware is built.
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
obj=$(mktemp)
trap 'rm -f "$obj"' EXIT HUP INT TERM

# Layout-relevant target flags from build_make/pico_rp2040.mk CFLAGS; -fcommon
# only keeps the probe arrays as sized symbols for nm.
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -ffreestanding -std=gnu11 -mfloat-abi=soft -fcommon \
    -c "$script_dir/sizeof_probe.c" -o "$obj"
arm-none-eabi-nm -S -t d --size-sort "$obj" |
    awk '$4 ~ /^probe_/ { sub(/^probe_/, "", $4); printf "%d %s\n", $2, $4 }'
