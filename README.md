# Autonomous Robotic Car

A C project for INF2004 using a **Raspberry Pi Pico WH mounted on a Cytron
Maker Pi Pico carrier board**, based on `Project Briefing.pdf`.
The car must follow a line, decode navigation barcodes, measure humps, bypass
obstacles, and report telemetry. Five subsystem owners share one vehicle controller.

**Start here:** [project guide](docs/GETTING_STARTED.md), then
[interfaces and ownership](docs/ARCHITECTURE.md).

## Implemented in code (physical hardware testing pending)

- Shared C data structures and a small controller with explicit start/stop behavior.
- Example proportional line steering using synthetic inputs.
- Stop behavior for obstacles, lost line, collision, invalid/stale readings,
  and navigation actions that have not been implemented.
- A scripted desktop demo, controller tests, and a Pico SDK build target.
- Pico WH onboard WiFi/MQTT telemetry, heartbeat, validated commands and reconnect handling.
- Separate source files for all five buddies, with sensor/motor drivers still to implement.

**Buddy 1 setup:** [WiFi and MQTT guide](docs/WIFI_TELEMETRY.md).

**This is a scaffold, not working autonomous-car firmware.** Sensor drivers,
motor PWM/encoder PID, barcode decoding, IMU processing, servo scanning,
avoidance/recovery are TODOs. Pico sensor stubs return invalid readings and motor
functions do nothing. Communications use the Pico WH onboard radio after local
network configuration; desktop builds use USB/console output. No external sensor
or motor GPIO pins are assigned yet. Radio/broker operation still needs a physical
bench test; successful compilation and mock tests do not establish hardware operation.

## Run without hardware (Windows PowerShell)

Prerequisites: GCC on PATH. No Pico SDK is needed for this desktop check.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/check.ps1
```

This compiles the shared controller and subsystem stubs, tests command parsing,
telemetry encoding and network recovery through a fake SDK, then prints a synthetic
mission sequence. It does not simulate vehicle physics or test a real radio/broker.

## Build for Pico WH

Pico WH uses the SDK's `pico_w` target, just like Pico W. The fitted headers do
not require another SDK board definition. The Maker Pi Pico is the carrier.

Install the Raspberry Pi Pico C/C++ SDK, its ARM toolchain, CMake, and Ninja.
Set `PICO_SDK_PATH` to your SDK checkout (including its submodules).
See the [official SDK setup](https://github.com/raspberrypi/pico-sdk).

```powershell
# First-time setup only; do not overwrite an existing configuration.
Copy-Item config/network_config.example.h config/network_config.h
# Edit config/network_config.h locally with WiFi and broker settings.
$env:PICO_SDK_PATH = 'C:\path\to\pico-sdk'
cmake -S . -B build-picow -G Ninja -DPICO_BOARD=pico_w
cmake --build build-picow
```

Alternatively, `powershell -ExecutionPolicy Bypass -File scripts/build-picow.ps1`
uses installed tools or the project-local tools in ignored `tmp/` when available.
The output is `build-picow/robot_car.uf2`. Hold BOOTSEL while connecting the board,
then copy the UF2 to its USB drive. Empty settings build successfully but leave
networking disabled when SSID is empty; an SSID with an empty broker IP enables
WiFi-only bring-up. The car starts in `IDLE`; sensor stubs prevent motion.

A standard Pico has **no onboard WiFi**. The confirmed Pico WH uses `pico_w`.
For a USB-only standard Pico build, use a separate build directory and
`-DPICO_BOARD=pico -DROBOT_ENABLE_WIFI=OFF`.
See [official board variants](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html).

## Layout

```text
include/robot/       Shared types, configuration, controller and subsystem APIs
src/controller.c    Mission decisions; never accesses GPIO
src/comms_protocol.c Shared telemetry encoding and validated command inbox
src/subsystems/     Buddy modules; comms_picow.c is the real WiFi/MQTT backend
src/main_pico.c     Cooperative Pico loop and USB command entry point
sim/main.c          Scripted inputs for a desktop demonstration
tests/              Controller behavior checks
scripts/check.ps1   Compile, test, and run on Windows with GCC
scripts/mqtt_client.py Laptop monitor/start/stop tool
config/             Network configuration example and local lab broker config
docs/               Build order, API contracts, pin planning and test evidence
```
