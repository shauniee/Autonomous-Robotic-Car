# CSC01 Autonomous Robotic Car

An autonomous Pico WH / Robo Pico robot for line tracking, barcode navigation, hump measurement, obstacle avoidance and WiFi telemetry. Firmware uses micro T-Kernel 3.0 on the RP2040. See [project specifications](docs/project-specs.md) for required outcomes and [Getting started](#getting-started) for what runs today.

## Contents

- [Quick start with Docker](#quick-start-with-docker)
- [Reading order](#reading-order)
- [Repository structure](#repository-structure)
- [Getting started](#getting-started)
  - [Build profiles](#build-profiles)
  - [Power and flashing](#power-and-flashing)
  - [Calibration and local controls](#calibration-and-local-controls)
  - [Wireless telemetry](#wireless-telemetry)
  - [Bring-up order and project tasks](#bring-up-order-and-project-tasks)
- [Build and flash without Docker](#build-and-flash-without-docker)

## Quick start with Docker

Docker supplies the Arm compiler and Pico SDK 2.2.0, so building needs nothing else installed. Flashing is done from your computer, not the container.

1. Install and start [Docker Desktop](https://docs.docker.com/get-started/get-docker/) (Linux: Docker Engine). Windows: use the WSL 2 backend and run the commands below in a WSL Ubuntu terminal.
2. Clone (private, access required) and open the repository root. In the coursework workspace, `code/` is the root.

   ```sh
   git clone https://github.com/shauniee/Autonomous-Robotic-Car.git
   cd Autonomous-Robotic-Car
   ```

3. Build the image once; rebuild it after `Dockerfile` changes:

   ```sh
   docker build -t robot-build .
   ```

4. Build the firmware with one of the four profiles below. Run builds one at a time because they share `build_make/`. TEST profiles inhibit the motors; normal profiles use `TEST=0`. See [Build profiles](#build-profiles) for option details and output filenames.

   **Offline TEST — motors inhibited**

   ```sh
   docker run --rm -v "$PWD":/work --user "$(id -u):$(id -g)" robot-build \
     make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=1
   ```

   **Offline normal**

   ```sh
   docker run --rm -v "$PWD":/work --user "$(id -u):$(id -g)" robot-build \
     make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=0
   ```

   Before building a wireless profile, create the local credentials header if it does not already exist:

   ```sh
   test -e config/wifi_secrets.h || cp config/wifi_secrets.example.h config/wifi_secrets.h
   ```

   Edit `config/wifi_secrets.h` with your `WIFI_SSID`, `WIFI_PASSWORD` and `MQTT_BROKER_IP` (the broker laptop's LAN IPv4 address). See [Wireless telemetry](#wireless-telemetry) for broker setup. The header is git-ignored; do not commit it. Wireless firmware embeds these credentials.

   **Wireless TEST — motors inhibited**

   ```sh
   docker run --rm -v "$PWD":/work --user "$(id -u):$(id -g)" robot-build \
     make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=1
   ```

   **Wireless normal**

   ```sh
   docker run --rm -v "$PWD":/work --user "$(id -u):$(id -g)" robot-build \
     make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=0
   ```

5. Flash: hold BOOTSEL while connecting USB, release when **RPI-RP2** appears and copy `build_make/mtk3pico_smp1_usb_cdc_wifi_test.uf2` to the drive. Continue with [Power and flashing](#power-and-flashing) for power rules and serial checks.

Prebuilt images: the `firmware` GitHub Actions workflow builds all four profiles on pushes to `main` and on pull requests and uploads the UF2 files as a run artifact. Its wireless images contain the placeholders from `config/wifi_secrets.example.h` and cannot join your network; build those yourself.

Docker and [native](#build-and-flash-without-docker) builds can share `build_make/`; a compiler change rebuilds the objects automatically.

## Reading order

| Guide | Use it for |
| --- | --- |
| [Project specifications](docs/project-specs.md) | Requirements, change requirement (minimum-bit), data table, assessment, pending decisions |
| [Team](docs/team.md) | Owners, pins and wiring, task and kernel-object budget, interfaces, coding rules |
| [Architecture](docs/architecture.md) | Tasks, authority, shared-data rules, MQTT protocol, subsystem navigation |
| [Subsystem guides](docs/architecture/) | `subsystem1.md` comms, `subsystem2.md` motion, `subsystem3.md` line/barcode/mission, `subsystem4.md` terrain, `subsystem5.md` obstacle |
| [Tribunal 1 summary](docs/additional-docs/tribunal1.md) | Week 8 defence, decisions and gaps |
| [Mission readiness checklist](testing/mission-readiness.md) | Ordered physical checks before a full run |
| [Week 13 plan](docs/week13-plan.md) | Deferred minimum-bit evidence and video recording |

Each fact has one owning guide; other guides link to it. Assignment requirements, proposed design, implemented behavior and recorded results are distinct.

## Repository structure

| Path | Contents |
| --- | --- |
| `app_program/` | `app_main.c` startup, shared definitions and five subsystem folders; application drivers stay with their subsystem |
| `build_make/` | Make profiles and generated firmware |
| `Dockerfile`, `.github/workflows/` | Docker build environment and the CI workflow that builds the UF2 images |
| `config/` | Kernel/device configuration and ignored local credentials |
| `kernel/`, `lib/`, `device/`, `include/`, `tools/` | Imported runtime, services, drivers, headers and conversion tools |
| `docs/` | Project guides, five subsystem guides, the Week 13 plan and one additional document |
| [testing/](testing/README.md) | Stable test IDs, result tables and saved evidence |
| `videos/` | Demonstration recordings (not committed); recording instructions in [week13-plan](docs/week13-plan.md#4-video-recording) |

## Getting started

Requirements are in [project-specs](docs/project-specs.md). Full-robot bring-up order is at the end of this section.

### Build profiles

Build sequentially (shared `build_make/`). Give every flag each time; incompatible objects rebuild automatically. With Docker, run each `make` command after `docker run ... robot-build` as in [Quick start](#quick-start-with-docker) step 4.

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

No active CMake target or dashboard build. Ordinary-Pico/Maker development uses the preserved handoff profile in [subsystem3](docs/architecture/subsystem3.md).

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
| Map | `python3 scripts/mqtt_client.py map <L> <R> <S> <U> --broker <ip>` while stopped | `MAPPING ACK ... OK`; telemetry shows `mapped=yes` |
| Start | Sensors on visible line, release then press GP20 | FOLLOW_LINE if mapped, calibration/data qualify and no start-blocking fault; otherwise refused and reported in telemetry. TEST still inhibits motors |
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

Laptop tool (binary protocol, schema v2):

```sh
python3 -m pip install -r scripts/requirements-mqtt.txt
python3 scripts/mqtt_client.py monitor --broker <broker-lan-ip>
python3 scripts/mqtt_client.py map A B C D --broker <broker-lan-ip>
python3 scripts/mqtt_client.py snapshot --broker <broker-lan-ip>
```

- `monitor` prints every telemetry frame, event and acknowledgement as text, including active fault names and the speed/direction derived from encoder counts.
- `map` sends the four mission-day letters in the order left, right, straight, U-turn and waits for the acknowledgement. The robot accepts it only while stopped, and a reboot clears it.
- `snapshot` requests one telemetry frame.
- `python3 scripts/mqtt_client.py --self-test` checks the decoder against the firmware golden vectors without a broker.

Optional auth via `ROBOT_MQTT_USERNAME` / `ROBOT_MQTT_PASSWORD`; `--root` / `--port` for other configs. Networking never starts, stops or drives the robot. Frame layouts: [architecture](docs/architecture.md#mqtt-binary-protocol-schema-version-2). The 500 ms stream cannot prove brief stop or recovery timing; broker loss does not change healthy local control.

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

## Build and flash without Docker

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

Build with the [Build profiles](#build-profiles) commands as written, from the repository root. `arm-none-eabi-gcc not found`: PATH wrong. Unset SDK path or missing USB/radio/network headers: fix SDK path/submodules.

Flash as in [Power and flashing](#power-and-flashing): hold BOOTSEL while connecting USB, release when **RPI-RP2** appears and copy the profile's UF2 from `build_make/` to the drive.
