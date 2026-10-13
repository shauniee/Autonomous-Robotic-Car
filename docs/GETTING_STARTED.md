# Getting started

## Current capabilities

S3 reads/calibrates two IR sensors and produces P steering. The edited bench guide describes an application line-following controller whose source/startup call is absent here; the [S3-to-S2 handoff](architecture/S3_TO_S2_HANDOFF.md#1-ownership-and-current-status) records that mismatch. Optional S1 reporting provides telemetry, heartbeat and status-only subscription through the kernel service, but its current S3 adapter is pending. S3 barcode buffering, provisional A–Z decoding and mapping/event APIs are implemented in software and **boot disabled** without measured configuration and an encoder/GPIO provider. See [Barcode](architecture/BARCODE.md). Encoder feedback, physical barcode capture, IMU/sonar acquisition and full Mission/Safety coordination remain pending. No successful physical tracking run, barcode scan or verified motor operation is established by compilation or retained software evidence.

Use the steps below for the existing bench. Full-robot tasks are listed at the end. Requirements live in [PROJECT_SPECS](PROJECT_SPECS.md); wiring in [TEAM](TEAM.md#2-pins).

## Install the tools

You need git, make, a host C++ compiler, the Pico C SDK and the separate **Arm bare-metal compiler** `arm-none-eabi-gcc`. Windows users run builds in WSL Ubuntu.

```sh
# macOS
xcode-select --install
```

```sh
# Ubuntu / WSL
sudo apt update
sudo apt install -y git make g++ gcc-arm-none-eabi
```

For macOS or a manually installed compiler, select the `arm-none-eabi` package matching the host from [Arm's toolchain downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads). WSL needs the Linux host package. Extract it and add its `bin` directory to PATH.

The instructor template uses SDK 2.2.0; SDK 2.3.0 is also recorded as working. Install the required submodules:

```sh
mkdir -p ~/pico
cd ~/pico
git clone --branch 2.2.0 https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init lib/tinyusb lib/cyw43-driver lib/lwip
```

Add the following to `~/.zshrc` or `~/.bashrc`, substituting your own paths. The Make build requires an SDK path **without spaces**; a symlink to a spaced path is suitable.

```sh
export PATH="$HOME/tools/arm-gnu-toolchain-<version>/bin:$PATH"
export PICO_SDK_PATH="$HOME/pico/pico-sdk"
```

Open a new terminal, then check:

```sh
arm-none-eabi-gcc --version
ls "$PICO_SDK_PATH/src"
```

Clone the private project with an account that has access:

```sh
cd ~/pico
git clone https://github.com/shauniee/Autonomous-Robotic-Car.git
cd Autonomous-Robotic-Car
```

In the surrounding coursework workspace, use `code/` as the repository root. All remaining commands run there.

## Build profiles

Build profiles **sequentially**: they share `build_make/`. Give every flag on each invocation; options are not remembered. Profile switching automatically rebuilds incompatible objects without `make clean`.

```sh
# USB-only development; motors always inhibited
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=1

# Normal offline bench
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=0

# Wireless development; motors always inhibited
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=1

# Normal wireless bench
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=0
```

| Profile | Output under `build_make/` |
| --- | --- |
| Offline TEST | `mtk3pico_smp1_usb_cdc_wifi_test.uf2` |
| Offline normal | `mtk3pico_smp1_usb_cdc_wifi.uf2` |
| Wireless TEST | `mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_test.uf2` |
| Wireless normal | `mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt.uf2` |

| Option | Meaning |
| --- | --- |
| `SMP=1` / `0` | Dual-core / single-core kernel; project profiles use dual core |
| `CONSOLE=usb_cdc` / `uart` | USB serial / UART0; UART uses encoder pins and is unsuitable with encoders attached |
| `WIFI=cyw43` / `none` | W/WH radio / no radio; ordinary Pico must use `none` |
| `NET=none` / `lwip`, `MQTT=0` / `1` | Offline / TCP-IP; MQTT requires lwIP and WiFi |
| `TEST=1` | Includes firmware self-tests and forces S2 braking even when the controller enables motion |
| `CRASH_REPORT=1` | Immediate crash/console copy on UART0; project uses `0` because of the [pin conflict](TEAM.md#wiring-conflicts) |

`arm-none-eabi-gcc not found` means PATH is wrong. An unset SDK path or missing USB/radio/network headers means the SDK path/submodules need fixing. There is no active CMake target or dashboard build. For ordinary-Pico/Maker development, use the preserved [handoff profile](architecture/IR_MOTION_HANDOFF.md#reproducible-software-builds).

## Power and flashing

**One power source at a time.** USB alone is for flashing and inhibited desktop checks. Disconnect USB before battery/power-bank operation; never add USB to collect logs during a powered trial. Reboot clears calibration, so calibrate again after changing power.

1. Check the [allocation and conflicts](TEAM.md#2-pins) and [electrical limits](TECHNICAL_INFO.md). The current bench uses motors, two line sensors, buttons and buzzer; barcode/IMU/sonar are not active and the servo signal is held low.
2. Hold BOOTSEL while connecting USB, release it when **RPI-RP2** appears, and copy the matching **TEST** UF2 onto the drive. On WSL, `explorer.exe build_make` exposes the files to Windows. The Pico reboots automatically.
3. Open the USB serial port at 115200 baud. List `/dev/tty.usbmodem*` on macOS or `/dev/ttyACM*` on Linux, then use the actual port:

   ```sh
   screen /dev/tty.usbmodem<device> 115200
   # Linux: screen /dev/ttyACM0 115200
   ```

   Windows can use PuTTY or Serial Monitor on the assigned COM port. Quit screen with Ctrl-A, then K.
4. Inspect startup, `[TEST] <ID> PASS|FAIL` and `[IR] ... SENSOR_ONLY` / `[LINE] ... MOTOR_INHIBITED`. Bench console output reports acquisition/calibration/state; it does not establish motor movement or measured duty. Record failures through [testing](../testing/README.md).
5. Before normal physical use, resolve the reported motor problem and record a raised-wheel baseline, polarity and stop response. Flash the normal image, disconnect USB, then connect the agreed battery/power bank alone. Use camera/scope and optional wireless observations for physical evidence.

## Calibration and local controls

Calibration is RAM-only. Use the sensor mounting and surface that will be used in the trial. Enable the buzzer's mute switch for audible feedback.

| Step | Action | Expected response |
| --- | --- | --- |
| Black | Start stopped/Ready, release then press GP20 with both sensors over black; hold the pose through capture | One chirp on completed capture |
| White | Release then press GP20 with both sensors over white; hold the pose through capture | Two chirps for valid pair; three for rejection |
| Start | Place sensors on a visible line, release then press GP20 | FOLLOW_LINE if calibration/data qualify; TEST still inhibits motors |
| Stop | Press GP21 | Persistent stop and capture cancellation |
| Reset | Clear the cause, release GP21, freshly press GP20 | STOPPED → READY; valid calibration retained |
| Restart | Release GP20 and press again on a visible line | A separate fresh start; held buttons never restart |

Capture duration/contrast and classification are defined in [Line Tracking](architecture/LINE_TRACKING.md#calibration-workflow); movement/recovery rules are in the [handoff](architecture/IR_MOTION_HANDOFF.md#tracking-and-recovery). Starting on fully white or broad black is rejected. Reboot to replace a completed calibration; failed white contrast clears both references. A plausible raw ADC value does not prove the sensor is wired correctly. Calibration and motion controls are local; USB text commands and MQTT calibration/start/stop commands are retired.

If capture is rejected, keep the robot stopped and inspect sensor connection, height, lighting, surface and raw contrast. If the controller stops, record the reason before resetting. Correcting software state does not prove the wheels are stationary.

## Wireless telemetry

Use a reachable private 2.4 GHz LAN. The supplied lab broker allows anonymous access without TLS; keep it on the trusted bench network.

1. Copy credentials only if the local file is absent:

   ```sh
   test -e config/wifi_secrets.h || cp config/wifi_secrets.example.h config/wifi_secrets.h
   ```

   Edit `WIFI_SSID`, `WIFI_PASSWORD` and `MQTT_BROKER_IP` locally. For a broker that requires authentication, also set `MQTT_USERNAME` and `MQTT_PASSWORD`. The client-ID prefix receives a radio-MAC suffix. Existing local headers can omit new settings and use defaults. The broker IP is the laptop's LAN IPv4 address, not `127.0.0.1` or `0.0.0.0`. The file is ignored; do not commit or print its contents.
2. Install/start Mosquitto. On macOS:

   ```sh
   brew install mosquitto
   ipconfig getifaddr en0
   mosquitto -c config/mosquitto-lab.conf
   ```

   If WiFi uses another interface, obtain that interface's address. Linux/WSL can install `mosquitto` and `mosquitto-clients`; ensure the broker is reachable from the Pico, especially across WSL networking/firewall boundaries.
3. In a second terminal:

   ```sh
   mosquitto_sub -h 127.0.0.1 -p 1883 -t 'robot/s3/#' -v
   ```

4. Build/flash wireless TEST, using USB alone, and confirm messages actually originate from the Pico. After physical readiness is established, use normal firmware with battery alone and recalibrate.

The restored laptop helper can monitor heartbeat freshness and request read-only status:

```sh
python3 -m pip install -r scripts/requirements-mqtt.txt
python3 scripts/mqtt_client.py monitor --broker <broker-lan-ip>
python3 scripts/mqtt_client.py status --broker <broker-lan-ip>
```

Optional laptop authentication uses `ROBOT_MQTT_USERNAME` / `ROBOT_MQTT_PASSWORD` environment variables. Use `--root` / `--port` when configured differently. The helper subscribes before requesting status, ignores cached retained status as a live response, and times out when no live response arrives. Firmware accepts only `{"request":"status"}` on `robot/s3/request`; start, stop, drive and calibration messages are rejected. A status request never changes local control.

[Communications](architecture/COMMUNICATIONS.md#mqtt-and-payload-contract) defines the topic, schema, fields, limits and reconnect behavior. Use validity/age flags when interpreting readings. Requested duty is not measured output; `motion_enabled` never overrides TEST inhibition. The slow telemetry stream cannot prove brief recovery or stop timing. Broker loss does not change healthy local control.

## Project tasks

| Order | Task / evidence needed | Owner |
| --- | --- | --- |
| 1 | Verify carrier/module, wiring, power and sensor levels; establish inhibited startup and calibration | All / S3 |
| 2 | Diagnose motors; validate raised-wheel output, polarity, stop/reset limits and encoder counts/geometry | S2 / All    |
| 3 | Tune wheel PID and record straight/distance/turn accuracy; validate calibrated P line guidance at mounted height | S2 / S3 |
| 4 | Capture known barcode patterns, reject invalid reads and implement one-shot navigation events | S3 |
| 5 | Restore IMU wiring; calibrate axes and validate hump detection/height against measured references | S4 |
| 6 | Verify sonar/servo; implement bounded coarse/fine profiles with independent forward freshness | S5 |
| 7 | Integrate proposed Mission/Safety contracts, action leases, scan preconditions, bypass and original-line recovery |  All |
| 8 | Integrate final mission telemetry; validate restored status subscription/recovery on Pico and measure network-load isolation | S1 / All |
| 9 | Record subsystem, integration and full mission acceptance; rehearse demonstration and confirm instructor access | All |

Resolve unknown values in [PROJECT_SPECS](PROJECT_SPECS.md#pending-decisions). Use [testing](../testing/README.md) for stable cases/results and [videos](../videos/README.md) for real-hardware evidence.
