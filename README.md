# CSC01 Autonomous Robotic Car

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

Start with the [consolidated completion plan](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) in issue #117. The core software is implemented; physical calibration, timing and full mission acceptance remain incomplete. Unresolved limits live in [pending decisions](docs/project-specs.md#pending-decisions).

An autonomous Pico WH / Robo Pico robot for line tracking, barcode navigation, hump measurement, obstacle avoidance and WiFi telemetry. Firmware uses micro T-Kernel 3.0 on the RP2040. See [project specifications](docs/project-specs.md) for required outcomes and [Getting started](#getting-started) for what runs today.

## Quick start with Docker

Docker supplies Mosquitto, the Python dashboard, the Arm compiler and Pico SDK 2.2.0. Flashing is done from your computer, not the container.

1. Install and start [Docker Desktop](https://docs.docker.com/get-started/get-docker/) (Linux: Docker Engine). Windows: use the WSL 2 backend and run the commands below in a WSL Ubuntu terminal.
2. Clone (private, access required) and open the repository root. In the coursework workspace, `code/` is the root.

   ```sh
   git clone https://github.com/shauniee/Autonomous-Robotic-Car.git
   cd Autonomous-Robotic-Car
   ```

3. Create the local credentials header, then edit `WIFI_SSID`, `WIFI_PASSWORD` and
   `MQTT_BROKER_IP` (your Mac's LAN IPv4 address, from `ipconfig getifaddr en0`
   or Wi-Fi settings). Use a reachable private 2.4 GHz network. Keep the header
   local; never commit it. Rebuild firmware after changing it.

   ```sh
   ipconfig getifaddr en0
   ```

   ```sh
   test -e config/wifi_secrets.h || cp config/wifi_secrets.example.h config/wifi_secrets.h
   docker compose up -d --build
   ```

   This starts Mosquitto on port 1883 and the dashboard at
   <http://127.0.0.1:8080>. Open that exact address for barcode mapping.
   Containers connect to `mosquitto`; the Pico connects to your Mac's LAN IP,
   never `127.0.0.1`, `0.0.0.0` or the Docker service name.
   The broker allows anonymous access without TLS: trusted bench network only.
   No local Mosquitto or Python installation is needed.

4. Build wireless TEST firmware (motors inhibited):

   ```sh
   docker compose run --rm --build --user "$(id -u):$(id -g)" firmware
   ```

   Builds share `build_make/`; run them one at a time. Other profiles use explicit
   overrides (all retain the existing build toolchain):

   ```sh
   # Wireless normal
   docker compose run --rm --build --user "$(id -u):$(id -g)" firmware make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=0
   # Offline TEST
   docker compose run --rm --build --user "$(id -u):$(id -g)" firmware make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=1
   # Offline normal
   docker compose run --rm --build --user "$(id -u):$(id -g)" firmware make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=0
   ```

5. For initial wireless TEST checks, use USB power alone. Hold BOOTSEL while
   connecting USB, release when **RPI-RP2** appears and copy
   `build_make/mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_test.uf2` to the drive.
   Flashing is a host USB file copy. See [Build profiles](#build-profiles) for
   other filenames and [Power and flashing](#power-and-flashing) before normal runs.

Optional terminal monitoring, decoder self-test, logs and shutdown:

```sh
docker compose exec dashboard python /app/mqtt_client.py monitor --broker mosquitto
docker compose exec dashboard python /app/mqtt_client.py --self-test
docker compose run --rm --build --user "$(id -u):$(id -g)" firmware sh testing/pure/run.sh
docker compose logs -f
docker compose down
```

The monitor is another view of the same messages, not another service. Other
client actions (`map`, `snapshot`) also run through `docker compose exec dashboard
python /app/mqtt_client.py ... --broker mosquitto`. The dashboard mounts the common
firmware headers read-only so decoding uses the checkout's configured scales.

If port 1883 is occupied, check `lsof -nP -iTCP:1883 -sTCP:LISTEN` and stop the
previous broker before starting Compose. For port 8080, stop the previous dashboard.
If the Pico cannot connect, check the laptop IP, firewall TCP 1883 access, Wi-Fi
credentials and guest-network device isolation. `docker compose logs -f mosquitto
dashboard` shows connection problems. Docker Desktop must be running.

Prebuilt images: the `firmware` GitHub Actions workflow builds all four profiles on pushes to `main` and on pull requests and uploads the UF2 files as a run artifact. Its wireless images contain the placeholders from `config/wifi_secrets.example.h` and cannot join your network; build those yourself.

Docker and [native](#build-and-flash-without-docker) builds can share `build_make/`; a compiler change rebuilds the objects automatically.

## Reading order

| Guide | Use it for |
| --- | --- |
| [Implementation plan](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) | Single live plan and remaining acceptance |
| [Project specifications](docs/project-specs.md) | Requirements, change requirement (minimum-bit), data table, assessment, pending decisions |
| [Team](docs/team.md) | Owners, pins and wiring, task and kernel-object budget, interfaces, coding rules |
| [Architecture](docs/architecture.md) | Target three-task contracts, looping Mission, stop/freshness rules, shared data and MQTT protocol |
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
| `Dockerfile`, `.github/workflows/` | Docker build environment and manual workflow that builds UF2 images; automatic CI is disabled |
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

No active CMake target. The local checkpoint includes an optional dashboard; it is not the verified remote-main baseline. Offline profiles remain build/test tools; the target normal mission requires WiFi and MQTT before start.

### Power and flashing

**One power source at a time.** USB alone is for flashing and inhibited desktop checks. Disconnect USB before battery/power-bank operation; never add USB for logs during a powered trial. Reboot clears calibration; recalibrate after changing power.

1. Check pins and conflicts ([team](docs/team.md#2-pins)). The user confirms documented wiring; actual active software and physical acceptance depend on the exact flashed revision. See [task ownership](docs/architecture.md#tasks) and the exact flashed source.
2. Hold BOOTSEL while connecting USB; release when **RPI-RP2** appears; copy the matching **TEST** UF2 to the drive (WSL: `explorer.exe build_make`). The Pico reboots.
3. Open USB serial at 115200: `screen /dev/tty.usbmodem<device> 115200` (Linux `/dev/ttyACM0`; Windows PuTTY/Serial Monitor on the COM port). Quit screen: Ctrl-A, K.
4. Inspect startup, `[TEST] <ID> PASS|FAIL`, `[IR] ... SENSOR_ONLY` / `[LINE] ... MOTOR_INHIBITED`. Console output reports digital acquisition/state, not motor movement or measured duty. Record failures via [testing](testing/README.md).
5. Before normal physical use: verify motor/encoder polarity and record raised-wheel baseline and stop response. Flash the normal image, disconnect USB, connect the battery/power bank alone. Use camera/scope and optional wireless observations as physical evidence.

### Calibration and local controls

The table below describes the working software control flow; physical release is pending the plan's calibration gates. IMU calibration is RAM-only; verify digital IR with the real mounting and surface. Enable audible buzzer feedback using its mute switch.

Target startup/control instructions are owned by [architecture](docs/architecture.md#startup-and-local-controls): bounded boot IMU calibration attempt; digital IR needs no software calibration; WiFi/MQTT required at start; eligible GP21 release returns READY, then fresh GP20 starts. This reset behavior is implemented in the working source.

| Step | Action | Expected response |
| --- | --- | --- |
| Map | `python3 scripts/mqtt_client.py map <L> <R> <S> <U> --broker <ip>` while stopped | `MAPPING ACK ... OK`; telemetry shows `mapped=yes` |
| Start | Sensors on visible line, release then press GP20 | FOLLOW_LINE if mapped, fresh digital IR/data qualify and no start-blocking fault; otherwise refused and reported in telemetry. TEST still inhibits motors |
| Stop | Press GP21 | Persistent stop; barcode scanning stopped |
| Reset | Clear cause; freshly press GP21, then release after braking is serviced | STOPPED to READY; IMU calibration kept; no movement. A separate fresh GP20 press starts |
| Restart | Release GP20, press again on a visible line | Fresh start; held buttons never restart |

Digital classification: [subsystem3](docs/architecture/subsystem3.md). No IR
black/white capture or calibration presses are needed. Both white means forward;
both black means junction and cannot start a run. Check DOUT wiring, black
polarity and comparator thresholds before physical use. Controls are local;
GP20 starts and GP21 stops/resets. If stopped, record the reason before reset.
Software state does not prove the wheels are stationary.

### Wireless telemetry

Use a reachable private 2.4 GHz LAN; the lab broker allows anonymous access without TLS, so keep it on the trusted bench network.

1. `test -e config/wifi_secrets.h || cp config/wifi_secrets.example.h config/wifi_secrets.h`; edit `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_BROKER_IP` (laptop LAN IPv4, not `127.0.0.1`/`0.0.0.0`); for authenticated brokers also `MQTT_USERNAME`, `MQTT_PASSWORD`. Client-ID prefix gets a radio-MAC suffix; older local headers may omit new settings. The file is git-ignored; do not commit or print it.

2. Start Docker Desktop, then `docker compose up -d --build` from the repository root.
3. Open <http://127.0.0.1:8080> or run the optional terminal monitor below.
4. Flash wireless TEST on USB alone and confirm messages originate from the Pico. After physical readiness, use normal firmware on battery alone and recalibrate.

Laptop tool in Docker (binary protocol, schema v2):

```sh
docker compose exec dashboard python /app/mqtt_client.py monitor --broker mosquitto
docker compose exec dashboard python /app/mqtt_client.py map A B C D --broker mosquitto
docker compose exec dashboard python /app/mqtt_client.py snapshot --broker mosquitto
```

For a non-Docker setup, install Mosquitto and the Python requirements locally:
macOS `brew install mosquitto`, then `$(brew --prefix)/sbin/mosquitto -c config/mosquitto-lab.conf`;
Linux/WSL install `mosquitto` and `mosquitto-clients` and allow Pico access through
firewall/WSL boundaries. Install `scripts/requirements-mqtt.txt` in a Python virtual
environment and run `python3 scripts/mqtt_client.py monitor --broker <broker-lan-ip>`.
Do not run a local broker alongside Compose on the same port.

- `monitor` prints every telemetry frame, event and acknowledgement as text, including active fault names and the speed/direction derived from encoder counts.
- `map` sends the four mission-day letters in the order left, right, straight, U-turn and waits for the acknowledgement. The robot accepts it only while stopped, and a reboot clears it.
- `snapshot` requests one telemetry frame.
- `python3 scripts/mqtt_client.py --self-test` checks the decoder against the firmware golden vectors without a broker.
- The dashboard has **Junction testing** buttons: install a barcode mapping, start
  locally with GP20, then click **Activate left/right/straight/U-turn** while
  FOLLOW_LINE, before the junction. Wait for the robot ACK; the direction takes
  priority over barcode reads and runs once at the next confirmed cross junction.
  GP21 cancels it. See [test policy](docs/project-specs.md#mission-day-barcode-rule).
- `python3 scripts/mqtt_client.py serve --broker <broker-lan-ip>` opens the live dashboard at `http://127.0.0.1:8080`. Its barcode-mapping strip sends the same mapping frame as `map` and shows the robot's acknowledgement; Send is enabled only while live telemetry reports READY or STOPPED. Open it at `127.0.0.1`, not `localhost`: the server refuses mapping requests from any other origin.

Optional auth via `ROBOT_MQTT_USERNAME` / `ROBOT_MQTT_PASSWORD`; `--root` / `--port` for other configs. Networking never starts, stops or drives the robot. Frame layouts: [architecture](docs/architecture.md#mqtt-binary-protocol-schema-version-2). The 500 ms stream cannot prove brief stop or recovery timing; broker loss does not change healthy local control.

### Bring-up order and project tasks

Follow the [consolidated issue plan](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117#remaining-plan) and the
[mission-readiness checklist](testing/mission-readiness.md). The three-task
Motion/Mission/I/O refactor is implemented; remaining work is calibration,
feature release, measured timing/load and mixed looping-mission acceptance.
No core swap or separate Safety task is introduced. Pending numeric values
remain in [project-specs](docs/project-specs.md#pending-decisions).

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

Software verification on 2026-10-29 is recorded in the [integration log](testing/integration/TEST_LOG.md#software-restructure--2026-10-29). Existing task periods, gains and checkpoint signs remain provisional software settings, not an instruction to flash an uncalibrated normal image. Offline profiles compile but cannot satisfy the normal WiFi/MQTT start prerequisite. Exactly three application tasks are created; see [architecture](docs/architecture.md#tasks).

Current continuation: obstacle triggering follows the resolved D16 decision in
[project specs](docs/project-specs.md#pending-decisions). Scan/filter settings
are configured as provisional software proposals, and physical bypass release
remains inhibited; encountering the trigger brakes and unsupported evidence stops
the manoeuvre. Do not interpret the passing synthetic
profile fixtures as an enabled full bypass. The user confirmed that physical
trials are unavailable; [full-plan completion](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117#done-when)
remains BLOCKED.
