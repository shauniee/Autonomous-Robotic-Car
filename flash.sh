#!/bin/sh
# Builds the firmware and copies it to the Pico.
#   ./flash.sh        full course: motors on, junction turns and bypass released
#   ./flash.sh test   wireless self-test image: motors always inhibited
# Hold BOOTSEL while plugging in USB; the script waits for the RPI-RP2 drive.
set -eu

repo=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tools=${ROBOT_TOOLS:-$HOME/.robot-car-tools}
drive=/Volumes/RPI-RP2
wait_seconds=60

case "${1:-course}" in
    course) profile="TEST=0 FULL_COURSE=1"; image=mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_course.uf2 ;;
    test) profile="TEST=1 FULL_COURSE=0"; image=mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_test.uf2 ;;
    *) echo "Usage: ./flash.sh [course|test]" >&2; exit 2 ;;
esac

[ -x "$tools/arm/bin/arm-none-eabi-gcc" ] && [ -d "$tools/pico-sdk" ] || {
    echo "Build tools not found. Run ./install_dependancies.sh first." >&2
    exit 1
}
[ -f "$repo/config/wifi_secrets.h" ] || {
    echo "Copy config/wifi_secrets.example.h to config/wifi_secrets.h and fill it in first." >&2
    exit 1
}
PATH="$tools/arm/bin:$PATH"
PICO_SDK_PATH="$tools/pico-sdk"
export PATH PICO_SDK_PATH

# shellcheck disable=SC2086 # profile is several make variables
make -C "$repo/build_make" -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 $profile
uf2=$repo/build_make/$image
[ -f "$uf2" ] || { echo "Build finished but $uf2 is missing." >&2; exit 1; }
shasum -a 256 "$uf2"

if [ ! -d "$drive" ]; then
    echo "Hold BOOTSEL while plugging in the Pico (waiting up to ${wait_seconds} s)..."
    waited=0
    while [ ! -d "$drive" ]; do
        [ "$waited" -lt "$wait_seconds" ] || {
            echo "No $drive drive appeared. The image is built: $uf2" >&2
            exit 1
        }
        sleep 1
        waited=$((waited + 1))
    done
fi

# The board reboots as the copy ends, which can make cp report an I/O error, so
# the drive disappearing is the proof of success.
cp -X "$uf2" "$drive/" 2>/dev/null || true
waited=0
while [ -d "$drive" ]; do
    [ "$waited" -lt 15 ] || {
        echo "$drive is still mounted: the copy did not take. Replug in BOOTSEL and retry." >&2
        exit 1
    }
    sleep 1
    waited=$((waited + 1))
done
echo "Flashed $image; the Pico has rebooted."
