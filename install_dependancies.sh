#!/bin/sh
# Installs what building, flashing and the laptop tools need on an Apple Silicon
# Mac. Safe to run again. Undo everything with ./delete_dependancies.sh.
#
# Everything except Homebrew packages lives in one folder; no sudo, no shell
# config changes. flash.sh finds the tools there on its own.
set -eu

repo=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tools=${ROBOT_TOOLS:-$HOME/.robot-car-tools}
arm_version=13.3.rel1 # closest Arm release to the Ubuntu 13.2 the earlier builds used
arm_name=arm-gnu-toolchain-$arm_version-darwin-arm64-arm-none-eabi
arm_url=https://developer.arm.com/-/media/Files/downloads/gnu/$arm_version/binrel/$arm_name.tar.xz
sdk_version=2.2.0
python_formula=python@3.12 # scripts/mqtt_client.py needs 3.10+; macOS ships 3.9

[ "$(uname -s)-$(uname -m)" = Darwin-arm64 ] || {
    echo "This installer is for Apple Silicon macOS only." >&2
    exit 1
}
command -v brew >/dev/null || {
    echo "Homebrew is required: https://brew.sh" >&2
    exit 1
}
xcode-select -p >/dev/null 2>&1 || {
    echo "Run 'xcode-select --install' first (make, c++, git), then run this again." >&2
    exit 1
}
mkdir -p "$tools"

# Only what this script adds, dependencies included, is recorded for removal.
before=$(brew list --formula -1 | sort)
for formula in mosquitto "$python_formula"; do
    brew list --formula "$formula" >/dev/null 2>&1 || brew install "$formula"
done
brew list --formula -1 | sort > "$tools/.brew-after"
printf '%s\n' "$before" | comm -13 - "$tools/.brew-after" >> "$tools/brew-installed.txt"
rm -f "$tools/.brew-after"

if [ ! -x "$tools/arm/bin/arm-none-eabi-gcc" ]; then
    echo "Downloading $arm_name (about 150 MB)"
    archive="$tools/$arm_name.tar.xz"
    curl -fL --retry 3 -o "$archive" "$arm_url"
    expected=$(curl -fsL --retry 3 "$arm_url.sha256asc" | awk '{print $1}')
    actual=$(shasum -a 256 "$archive" | awk '{print $1}')
    [ -n "$expected" ] && [ "$expected" = "$actual" ] || {
        echo "Checksum mismatch for $archive: expected '$expected', got '$actual'" >&2
        rm -f "$archive"
        exit 1
    }
    rm -rf "$tools/arm" "$tools/arm.tmp"
    mkdir "$tools/arm.tmp"
    tar -xJf "$archive" -C "$tools/arm.tmp" --strip-components 1
    mv "$tools/arm.tmp" "$tools/arm"
    rm -f "$archive"
fi

if [ ! -f "$tools/pico-sdk/pico_sdk_init.cmake" ]; then
    rm -rf "$tools/pico-sdk"
    git clone --depth 1 --branch "$sdk_version" \
        https://github.com/raspberrypi/pico-sdk.git "$tools/pico-sdk"
fi
git -C "$tools/pico-sdk" submodule update --init --depth 1 lib/tinyusb lib/cyw43-driver lib/lwip

[ -x "$tools/venv/bin/python" ] ||
    "$(brew --prefix)/opt/$python_formula/bin/python3.12" -m venv "$tools/venv"
"$tools/venv/bin/python" -m pip install --quiet --disable-pip-version-check \
    -r "$repo/scripts/requirements-mqtt.txt"

"$tools/arm/bin/arm-none-eabi-gcc" --version | head -1
echo "Pico SDK $sdk_version: $tools/pico-sdk"
"$tools/venv/bin/python" -c 'import paho.mqtt, sys; print("Python", sys.version.split()[0], "paho-mqtt", paho.mqtt.__version__)'
echo "Done. Next: ./flash.sh"
