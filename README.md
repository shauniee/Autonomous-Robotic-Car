# CSC01 Autonomous Robotic Car

An autonomous Pico WH / Robo Pico robot for line tracking, barcode navigation, hump measurement, obstacle avoidance and WiFi telemetry. Firmware uses micro T-Kernel 3.0 on the RP2040. See [project specifications](docs/project-specs.md) for required outcomes and [Getting started](#getting-started) for what runs today.

## Reading order

| Guide | Use it for |
| --- | --- |
| [Project specifications](docs/project-specs.md) | Requirements, change requirement (minimum-bit), data table, assessment, pending decisions |
| [Team](docs/team.md) | Owners, pins and wiring, task and kernel-object budget, interfaces, coding rules |
| [Architecture](docs/architecture.md) | Tasks, authority, shared-data rules, MQTT protocol, subsystem navigation |
| [Subsystem guides](docs/architecture/) | `subsystem1.md` comms, `subsystem2.md` motion, `subsystem3.md` line/barcode/mission, `subsystem4.md` terrain, `subsystem5.md` obstacle |
| [Tribunal 1 summary](docs/additional-docs/tribunal1.md) | Week 8 defence, decisions and gaps |

Each fact has one owning guide; other guides link to it. Assignment requirements, proposed design, implemented behavior and recorded results are distinct.

## Repository structure

| Path | Contents |
| --- | --- |
| `app_program/` | `app_main.c` startup, shared definitions and five subsystem folders; application drivers stay with their subsystem |
| `build_make/` | Make profiles and generated firmware |
| `config/` | Kernel/device configuration and ignored local credentials |
| `kernel/`, `lib/`, `device/`, `include/`, `tools/` | Imported runtime, services, drivers, headers and conversion tools |
| `docs/` | Three project guides, five subsystem guides and one additional document |
| [testing/](testing/README.md) | Stable test IDs, result tables and saved evidence |
| [videos/](videos/README.md) | Unlisted demonstration links |

## Getting started

Requirements are in [project-specs](docs/project-specs.md). Full-robot bring-up order is at the end of this section.

### Install tools

Needs git, make, host C++ compiler, Pico C SDK and `arm-none-eabi-gcc`. Windows: build in WSL Ubuntu.

```sh
xcode-select --install                                    # macOS
sudo apt update && sudo apt install -y git make g++ gcc-arm-none-eabi   # Ubuntu / WSL
```

For macOS/manual compilers use the matching `arm-none-eabi` package from Arm's GNU toolchain downloads (WSL: Linux host package); extract and add `bin` to PATH. Template uses SDK 2.2.0 (2.3.0 also recorded working):

```sh
mkdir -p ~/pico && cd ~/pico
git clone --branch 2.2.0 https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk && git submodule update --init lib/tinyusb lib/cyw43-driver lib/lwip
```

In `~/.zshrc` or `~/.bashrc` (SDK path must have **no spaces**; a symlink works):

```sh
export PATH="$HOME/tools/arm-gnu-toolchain-<version>/bin:$PATH"
export PICO_SDK_PATH="$HOME/pico/pico-sdk"
```

Check: `arm-none-eabi-gcc --version`; `ls "$PICO_SDK_PATH/src"`. Clone (private, access required): `git clone https://github.com/shauniee/Autonomous-Robotic-Car.git`. In the coursework workspace, `code/` is the repository root; run commands there.

### Build profiles

Build sequentially (shared `build_make/`). Give every flag each time; incompatible objects rebuild automatically.

```sh
# USB-only dev; motors always inhibited
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=1
# Normal offline bench: same with TEST=0
# Wireless dev (motors inhibited)
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=1
# Normal wireless bench: same with TEST=0
```

| Profile | Output in `build_make/` |
| --- | --- |
| Offline TEST | `mtk3pico_smp1_usb_cdc_wifi_test.uf2` |
| Offline normal | `mtk3pico_smp1_usb_cdc_wifi.uf2` |
| Wireless TEST | `mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_test.uf2` |
| Wireless normal | `mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt.uf2` |

| Option | Meaning |
| --- | --- |
| `SMP=1`/`0` | Dual/single-core kernel; project uses dual |
| `CONSOLE=usb_cdc`/`uart` | USB serial / UART0 (uses encoder pins; unsuitable with encoders) |
| `WIFI=cyw43`/`none` | W/WH radio / none (ordinary Pico must use `none`) |
| `NET=none`/`lwip`, `MQTT=0`/`1` | Offline / TCP-IP; MQTT needs lwIP + WiFi |
| `TEST=1` | Firmware self-tests; forces S2 braking even if the controller enables motion |
| `CRASH_REPORT=1` | Crash/console copy on UART0; project uses `0` (GP0/GP1 conflict, [pins](docs/team.md#2-pins)) |

`arm-none-eabi-gcc not found`: PATH wrong. Unset SDK path or missing USB/radio/network headers: fix SDK path/submodules. No active CMake target or dashboard build. Ordinary-Pico/Maker development uses the preserved handoff profile in [subsystem3](docs/architecture/subsystem3.md).

### Power and flashing

**One power source at a time.** USB alone is for flashing and inhibited desktop checks. Disconnect USB before battery/power-bank operation; never add USB for logs during a powered trial. Reboot clears calibration; recalibrate after changing power.

1. Check pins and conflicts ([team](docs/team.md#2-pins)). Current bench: motors, two line sensors, buttons, buzzer; barcode/IMU/sonar not active; servo signal held low.
2. Hold BOOTSEL while connecting USB; release when **RPI-RP2** appears; copy the matching **TEST** UF2 to the drive (WSL: `explorer.exe build_make`). The Pico reboots.
3. Open USB serial at 115200: `screen /dev/tty.usbmodem<device> 115200` (Linux `/dev/ttyACM0`; Windows PuTTY/Serial Monitor on the COM port). Quit screen: Ctrl-A, K.
4. Inspect startup, `[TEST] <ID> PASS|FAIL`, `[IR] ... SENSOR_ONLY` / `[LINE] ... MOTOR_INHIBITED`. Console output reports acquisition/calibration/state, not motor movement or measured duty. Record failures via [testing](testing/README.md).
5. Before normal physical use: resolve the reported motor problem; record raised-wheel baseline, polarity and stop response. Flash the normal image, disconnect USB, connect the battery/power bank alone. Use camera/scope and optional wireless observations as physical evidence.

### Calibration and local controls

Calibration is RAM-only; use the real mounting and surface. Enable the buzzer mute switch for audible feedback.

| Step | Action | Expected response |
| --- | --- | --- |
| Black | Stopped/Ready, release then press GP20 with both sensors over black; hold through capture | One chirp on completed capture |
| White | Release then press GP20 over white; hold through capture | Two chirps valid pair; three rejection |
| Start | Sensors on visible line, release then press GP20 | FOLLOW_LINE if calibration/data qualify; TEST still inhibits motors |
| Stop | Press GP21 | Persistent stop; capture cancelled |
| Reset | Clear cause, release GP21, freshly press GP20 | STOPPED to READY; valid calibration kept |
| Restart | Release GP20, press again on a visible line | Fresh start; held buttons never restart |

Capture duration/contrast and classification: [subsystem3](docs/architecture/subsystem3.md). Starting on fully white or broad black is rejected. Reboot replaces a completed calibration; failed white contrast clears both references. A plausible raw ADC value does not prove correct wiring. Controls are local; USB text commands and MQTT calibration/start/stop are retired. If capture is rejected, keep stopped and check connection, height, lighting, surface, raw contrast. If the controller stops, record the reason before reset. Software state does not prove the wheels are stationary.

### Wireless telemetry

Use a reachable private 2.4 GHz LAN; the lab broker allows anonymous access without TLS, so keep it on the trusted bench network.

1. `test -e config/wifi_secrets.h || cp config/wifi_secrets.example.h config/wifi_secrets.h`; edit `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_BROKER_IP` (laptop LAN IPv4, not `127.0.0.1`/`0.0.0.0`); for authenticated brokers also `MQTT_USERNAME`, `MQTT_PASSWORD`. Client-ID prefix gets a radio-MAC suffix; older local headers may omit new settings. The file is git-ignored; do not commit or print it.
2. macOS: `brew install mosquitto`, `ipconfig getifaddr en0` (use the WiFi interface's address if different), `mosquitto -c config/mosquitto-lab.conf`. Linux/WSL: `mosquitto` + `mosquitto-clients`; ensure Pico reachability across WSL/firewall boundaries.
3. Second terminal: `mosquitto_sub -h 127.0.0.1 -p 1883 -t 'robot/s3/#' -v`
4. Flash wireless TEST on USB alone and confirm messages originate from the Pico. After physical readiness, use normal firmware on battery alone and recalibrate.

Laptop helper (heartbeat freshness, read-only status):

```sh
python3 -m pip install -r scripts/requirements-mqtt.txt
python3 scripts/mqtt_client.py monitor --broker <broker-lan-ip>
python3 scripts/mqtt_client.py status --broker <broker-lan-ip>
```

Optional auth via `ROBOT_MQTT_USERNAME` / `ROBOT_MQTT_PASSWORD`; `--root` / `--port` for other configs. The helper subscribes before requesting status, ignores cached retained status as live, and times out without a live response. Firmware accepts only `{"request":"status"}` on `robot/s3/request`; start/stop/drive/calibration messages are rejected; status requests never change local control. Topic, schema, fields, limits and reconnect: [subsystem1](docs/architecture/subsystem1.md). Use validity/age flags; requested duty is not measured output; `motion_enabled` never overrides TEST inhibition; slow telemetry cannot prove brief recovery or stop timing; broker loss does not change healthy local control.

### Bring-up order and project tasks

1. Verify carrier/module, wiring, power, sensor levels; inhibited startup and calibration (All / S3).
2. Diagnose motors; raised-wheel output, polarity, stop/reset limits, encoder counts/geometry (S2 / All).
3. Tune wheel PID; straight/distance/turn accuracy; calibrated P line guidance at mounted height (S2 / S3).
4. Capture known barcode patterns, reject invalid reads, implement one-shot navigation events (S3).
5. Restore IMU wiring; calibrate axes; validate hump detection/height against references (S4).
6. Verify sonar/servo; bounded coarse/fine profiles with independent forward freshness (S5).
7. Integrate proposed Mission/Safety contracts, action leases, scan preconditions, bypass, original-line recovery (All).
8. Integrate final mission telemetry; validate status subscription/recovery on Pico; measure network-load isolation (S1 / All).
9. Record subsystem, integration and full mission acceptance; rehearse demo; confirm instructor access (All).

Unknown values: [project-specs](docs/project-specs.md#pending-decisions).
