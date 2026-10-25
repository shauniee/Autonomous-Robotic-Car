#!/bin/sh
# Requires PICO_SDK_PATH and arm-none-eabi-gcc on PATH, e.g.
#   export PICO_SDK_PATH=/path/to/pico-sdk
set -e
cd "$(dirname "$0")/../build_make"
make -j4 SMP=1 CONSOLE=usb_cdc WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=1
