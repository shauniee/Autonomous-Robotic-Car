#!/bin/sh
# Compile every software profile the course and test images need.
# RUN ONLY in a configured RP2040/Pico SDK toolchain environment.
# No flashing, motor enable, or claiming of hardware acceptance.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
: "${PICO_SDK_PATH:?Set PICO_SDK_PATH to your installed Pico SDK before running}"
command -v arm-none-eabi-gcc >/dev/null 2>&1 || {
  echo "Missing arm-none-eabi-gcc (required to link target firmware)" >&2
  exit 1
}
command -v make >/dev/null 2>&1 || {
  echo "Missing make" >&2
  exit 1
}
jobs=${JOBS:-4}
case "$jobs" in
  ''|*[!0-9]*) echo "JOBS must be a positive integer" >&2; exit 1 ;;
  0) echo "JOBS must be positive" >&2; exit 1 ;;
esac

printf 'Source commit: '
git -C "$root" rev-parse HEAD
printf 'Compiler: '
arm-none-eabi-gcc -dumpfullversion
echo "No firmware will be flashed by this script."
# Bench matrix: both trace settings for normal and TEST. The shared build
# objects are rebuilt on profile change by pico_rp2040.mk.
for profile in '0 0' '0 1' '1 0' '1 1'; do
  set -- $profile
  test_mode=$1 trace_mode=$2
  printf '\n=== TEST=%s TRACE=%s FULL_COURSE=0 ===\n' "$test_mode" "$trace_mode"
  make -C "$root/build_make" -j"$jobs" SMP=1 CONSOLE=usb_cdc \
    WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 \
    TEST="$test_mode" FULL_COURSE=0 NOGUARDS=0 \
    TRACE="$trace_mode"
done
# The images flash.sh course and trial build; only these turn the stall guard on.
for trace_mode in 0 1; do
  printf '\n=== TEST=0 FULL_COURSE=1 TRACE=%s ===\n' "$trace_mode"
  make -C "$root/build_make" -j"$jobs" SMP=1 CONSOLE=usb_cdc \
    WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 \
    TEST=0 FULL_COURSE=1 NOGUARDS=0 TRACE="$trace_mode"
done
# The images flash.sh bench, bench10 and benchturn build.
for bench_mode in 1 2 3; do
  printf "\n=== TEST=0 FULL_COURSE=1 TRACE=1 BENCH=%s ===\n" "$bench_mode"
  make -C "$root/build_make" -j"$jobs" SMP=1 CONSOLE=usb_cdc \
    WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 \
    TEST=0 FULL_COURSE=1 NOGUARDS=0 TRACE=1 BENCH="$bench_mode"
done
# Target struct sizes; the probe's static asserts fail if a narrowed type grows.
printf '\n=== Target struct sizes ===\n'
sh "$root/testing/tools/sizeof_probe.sh" >/dev/null
echo "Struct size bounds hold."
echo "All requested *compilation* profiles finished; physical tests are still pending."
