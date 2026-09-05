# Autonomous Robotic Car

A C starter project for a Raspberry Pi Pico, based on `Project Briefing.pdf`.
The car must follow a line, decode navigation barcodes, measure humps, bypass
obstacles, and report telemetry. Five subsystem owners share one vehicle controller.

**Start here:** [project guide](docs/GETTING_STARTED.md), then
[interfaces and ownership](docs/ARCHITECTURE.md).

## What works today

- Shared C data structures and a small controller with explicit start/stop behavior.
- Example proportional line steering using synthetic inputs.
- Stop behavior for obstacles, lost line, collision, invalid/stale readings,
  and navigation actions that have not been implemented.
- A scripted desktop demo, controller tests, and a Pico SDK build target.
- Separate source files for all five buddies, ready to replace with real drivers.

**This is a scaffold, not working autonomous-car firmware.** Sensor drivers,
motor PWM/encoder PID, barcode decoding, IMU processing, servo scanning,
avoidance/recovery, WiFi, and MQTT are TODOs. Pico sensor stubs return invalid
readings, motor functions do nothing, and communications produce USB serial
status only. No GPIO pins are assigned yet.

## Run without hardware (Windows PowerShell)

Prerequisites: GCC on PATH. No Pico SDK is needed for this desktop check.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/check.ps1
```

This compiles the shared controller and all subsystem stubs, runs the tests,
then prints a synthetic mission sequence. It does not simulate vehicle physics.

## Build for Pico

Install the Raspberry Pi Pico C/C++ SDK, its ARM toolchain, CMake, and Ninja.
Set `PICO_SDK_PATH` to your SDK checkout (including its submodules).
See the [official SDK setup](https://github.com/raspberrypi/pico-sdk).

```powershell
$env:PICO_SDK_PATH = 'C:\path\to\pico-sdk'
cmake -S . -B build-pico -G Ninja -DPICO_BOARD=pico
cmake --build build-pico
```

The intended output is `build-pico/robot_car.uf2`. Hold BOOTSEL while connecting
the board, then copy the UF2 to its USB drive. The starter reports `IDLE` over USB
serial and will not drive a motor. CMake requires a working SDK/toolchain setup;
the desktop check alone does not verify the firmware build.

A standard Pico has **no onboard WiFi**. For Buddy 1, confirm whether you actually
have a Pico W/WH or need an external WiFi module. A Pico W build uses
`-DPICO_BOARD=pico_w`; selecting that board does not implement networking.
See [official board variants](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html).

## Layout

```text
include/robot/       Shared types, configuration, controller and subsystem APIs
src/controller.c    Mission decisions; never accesses GPIO
src/subsystems/     One implementation file per buddy
src/main_pico.c     Cooperative Pico loop and USB command entry point
sim/main.c          Scripted inputs for a desktop demonstration
tests/              Controller behavior checks
scripts/check.ps1   Compile, test, and run on Windows with GCC
docs/               Build order, API contracts, pin planning and test evidence
```
