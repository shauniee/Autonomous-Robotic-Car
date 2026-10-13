#!/bin/sh
set -e
export PATH="/c/ProgramData/chocolatey/bin:/c/PROGRA~1/RASPBE~2/PICOSD~1.1/GCC-AR~1/bin:/c/PROGRA~1/RASPBE~2/PICOSD~1.1/PICO-S~1:$PATH"
export PICO_SDK_PATH="/c/PROGRA~1/RASPBE~2/PICOSD~1.1/pico-sdk"
cd "$(dirname "$0")/../build_make"
make -j4 SMP=1 CONSOLE=usb_cdc WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=1
