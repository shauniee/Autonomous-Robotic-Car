# CSC01 Autonomous Robotic Car

## Contents

- [Quick start](#quick-start)
- [Reading order](#reading-order)
- [Repository structure](#repository-structure)
- [Getting started](#getting-started)
  - [Build profiles](#build-profiles)
  - [Power and flashing](#power-and-flashing)
  - [Calibration and local controls](#calibration-and-local-controls)
  - [Wireless telemetry](#wireless-telemetry)
  - [Bring-up order and project tasks](#bring-up-order-and-project-tasks)
- [Bits per subsystem](#bits-per-subsystem)
- [Manual build and flash](#manual-build-and-flash)

Start with the [consolidated completion plan](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) in issue #117. The core software is implemented; physical calibration, timing and full mission acceptance remain incomplete. Unresolved limits live in [pending decisions](docs/project-specs.md#pending-decisions).

An autonomous Pico WH / Robo Pico robot for line tracking, barcode navigation, hump measurement, obstacle avoidance and WiFi telemetry. Firmware uses micro T-Kernel 3.0 on the RP2040. See [project specifications](docs/project-specs.md) for required outcomes and [Getting started](#getting-started) for what runs today.

## Quick start

For an Apple Silicon Mac with [Homebrew](https://brew.sh). Other systems: [Manual build and flash](#manual-build-and-flash).

1. Install the Arm compiler, Pico SDK 2.2.0, Mosquitto and the Python tools. Most go in `~/.robot-car-tools`; no `sudo` or shell changes.

   ```sh
   xcode-select --install   # if not installed yet
   ./install_dependancies.sh
   ```

2. Create the local credentials header, then edit `WIFI_SSID`, `WIFI_PASSWORD` and
   `MQTT_BROKER_IP` (your Mac's LAN IPv4 address, from `ipconfig getifaddr en0`). Use a
   reachable private 2.4 GHz network. Never commit this header; rebuild after changing it.

   ```sh
   test -e config/wifi_secrets.h || cp config/wifi_secrets.example.h config/wifi_secrets.h
   ```

3. Build and flash. The script waits for the drive: hold BOOTSEL while plugging in USB.

   ```sh
   ./flash.sh        # full course: motors on, junction turns and obstacle bypass released
   ./flash.sh test   # wireless self-test image: motors always inhibited
   ```

   See [Power and flashing](#power-and-flashing) before running on the floor.

4. Start the broker and dashboard, each in its own terminal, from the repository root.
   Open <http://127.0.0.1:8080> for barcode mapping. The broker allows anonymous access
   without TLS: trusted bench network only.

   ```sh
   "$(brew --prefix)/sbin/mosquitto" -c config/mosquitto-lab.conf
   ```

   ```sh
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py serve --broker 127.0.0.1
   ```

Terminal monitor and host tests:

```sh
~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py monitor --broker 127.0.0.1
```

```sh
testing/pure/run.sh
```

The Pico connects to your Mac's LAN IP, never `127.0.0.1` or `0.0.0.0`. If port 1883 is
occupied, check `lsof -nP -iTCP:1883 -sTCP:LISTEN` and stop the previous broker. If the
Pico cannot connect, check the laptop IP, firewall TCP 1883 access, Wi-Fi credentials
and guest-network device isolation.

When the project is over, `./delete_dependancies.sh` removes the tools folder and only
the Homebrew packages the installer added (`--dry-run` lists them first).

## Reading order

| Guide | Use it for |
| --- | --- |
| [Implementation plan](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) | Single live plan and remaining acceptance |
| [Project specifications](docs/project-specs.md) | Requirements, change requirement (minimum-bit), data table, assessment, pending decisions |
| [Team](docs/team.md) | Owners, pins and wiring, task and kernel-object budget, interfaces, coding rules |
| [Architecture](docs/architecture.md) | Target three-task contracts, looping Mission, stop/freshness rules, shared data and MQTT protocol |
| Subsystem READMEs | [comms](app_program/comms/README.md), [motion](app_program/motion/README.md), [line_barcode](app_program/line_barcode/README.md), [terrain](app_program/terrain/README.md), [obstacle](app_program/obstacle/README.md) |
| [Mission readiness checklist](testing/mission-readiness.md) | Ordered physical checks before a full run |
| [Videos](video/README.md) | Recording and sharing instructions, video index |

Each fact has one owning guide; other guides link to it. Assignment requirements, proposed design, implemented behavior and recorded results are distinct.

## Repository structure

| Path | Contents |
| --- | --- |
| `app_program/` | `app_main.c` startup, shared definitions, Mission and five subsystem folders, each with a README |
| `build_make/` | Make profiles and generated firmware |
| `flash.sh`, `install_dependancies.sh`, `delete_dependancies.sh` | Build and flash; install and remove the local tools |
| `config/` | Kernel/device configuration and ignored local credentials |
| `kernel/`, `lib/`, `device/`, `include/`, `tools/` | Imported runtime, services, drivers, headers and conversion tools |
| `docs/` | Project specifications, team and architecture |
| [testing/](testing/README.md) | Stable test IDs, result tables and saved evidence |
| [video/](video/README.md) | Recording instructions and index; video files are never committed |

## Getting started

Requirements are in [project-specs](docs/project-specs.md). Full-robot bring-up order is at the end of this section.

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
| `./flash.sh` | `full_course.uf2` |
| `./flash.sh test` | `test.uf2` |

| Option | Meaning |
| --- | --- |
| `SMP=1`/`0` | Dual/single-core kernel; project uses dual |
| `CONSOLE=usb_cdc`/`uart` | USB serial / UART0 (uses encoder pins; unsuitable with encoders) |
| `WIFI=cyw43`/`none` | W/WH radio / none (ordinary Pico must use `none`) |
| `NET=none`/`lwip`, `MQTT=0`/`1` | Offline / TCP-IP; MQTT needs lwIP + WiFi |
| `FULL_COURSE=1` | Releases junction turns and obstacle bypass; `_course` image (`flash.sh` default) |
| `TEST=1` | Firmware self-tests; forces S2 braking even if the controller enables motion |
| `WATCHDOG=1` | **Opt-in bench validation only:** arms a 2-second RP2040 reset watchdog after Motion and Mission report fresh progress; `_wd` image. Default `0` until hardware validation (D15) |
| `TRACE=0` | Skip high-rate diagnostic frame production (types 9/10/11, plus Terrain trace), retaining telemetry, events, ACKs and all safety; `_notrace` image. Default `TRACE=1` |
| `CRASH_REPORT=1` | Crash/console copy on UART0; project uses `0` (GP0/GP1 conflict, [pins](docs/team.md#2-pins)) |

For line-tracking A/B trials, keep the same course, battery condition and
controller settings, but build one `TRACE=1` image and one `TRACE=0` image.
Each build has a distinct profile name/hash; capture the same number of laps
and compare success rates and latency, not just how smooth the dashboard
looks. With `TRACE=0`, the dashboard still receives ordinary state/event
telemetry but not high-rate wheel/line/barcode/terrain graphs.

Watchdog policy (D15): `ROBOT_WATCHDOG_ENABLED=0` is the default; the
1 MHz timer tick alone does **not** enable reset protection. For a controlled
**wheels-raised** trial, build explicitly with `WATCHDOG=1 TEST=1` and
`FULL_COURSE=0 NOGUARDS=0`, then flash that distinct `_wd` image using
USB as the sole power source. Only the I/O task feeds it after both Motion
and Mission have reported new successful iterations, with a 250 ms
producer-freshness check. A hung producer or I/O task stops feeding; a
2-second hardware reset is intended. Validate actual reset timing, PWM/driver
pin behaviour during reset, boot braking, and lack of autonomous restart
before considering a driving build. This has **not** been physically accepted.

No active CMake target. The local checkpoint includes an optional dashboard; it is not the verified remote-main baseline. Offline profiles remain build/test tools; the target normal mission requires WiFi and MQTT before start.

### Power and flashing

**One power source at a time.** USB alone is for flashing and inhibited desktop checks. Disconnect USB before battery/power-bank operation; never add USB for logs during a powered trial. Reboot clears calibration; recalibrate after changing power.

1. Check pins and conflicts ([team](docs/team.md#2-pins)). The user confirms documented wiring; actual active software and physical acceptance depend on the exact flashed revision. See [task ownership](docs/architecture.md#tasks) and the exact flashed source.
2. Hold BOOTSEL while connecting USB; release when **RPI-RP2** appears; copy the matching **TEST** UF2 to the drive (WSL: `explorer.exe build_make`). The Pico reboots.
3. Open USB serial at 115200: `screen /dev/tty.usbmodem<device> 115200` (Linux `/dev/ttyACM0`; Windows PuTTY/Serial Monitor on the COM port). Quit screen: Ctrl-A, K.
4. Inspect startup, `[TEST] <ID> PASS|FAIL`, `[IR] ... SENSOR_ONLY` / `[LINE] ... MOTOR_INHIBITED`. Console output reports digital acquisition/state, not motor movement or measured duty. Record failures via [testing](testing/README.md).
5. Before normal physical use: verify motor/encoder polarity and record raised-wheel baseline and stop response. Flash the normal image, disconnect USB, connect the battery/power bank alone. Use camera/scope and optional wireless observations as physical evidence.

### Calibration and local controls

For motor tuning, use the [wheel-speed procedure](app_program/motion/README.md#wheel-speed-tuning).
The monitor's `WHEEL` records and dashboard target/PID-speed/duty readings
come from Motion releases; ordinary telemetry speed is a slower average.
Verify digital IR once, then hold steering settings fixed while checking
encoder speed response. The dashboard no longer asks for black/white captures.

The table below describes the working software control flow; physical release is pending the plan's calibration gates. IMU calibration is RAM-only; verify digital IR with the real mounting and surface.

Target startup/control instructions are owned by [architecture](docs/architecture.md#startup-and-local-controls): bounded boot IMU calibration attempt; digital IR needs no software calibration; WiFi/MQTT required at start; eligible GP21 release returns READY, then fresh GP20 starts. This reset behavior is implemented in the working source.

| Step | Action | Expected response |
| --- | --- | --- |
| Map | `python3 scripts/mqtt_client.py map <L> <R> <S> <U> --broker <ip>` while stopped | `MAPPING ACK ... OK`; telemetry shows `mapped=yes` |
| Start | Sensors on visible line, release then press GP20 | FOLLOW_LINE if mapped, fresh digital IR/data qualify and no start-blocking fault; otherwise refused and reported in telemetry. TEST still inhibits motors |
| Stop | Press GP21 | Persistent stop; barcode scanning stopped |
| Reset | Clear cause; freshly press GP21, then release after braking is serviced | STOPPED to READY; IMU calibration kept; no movement. A separate fresh GP20 press starts |
| Restart | Release GP20, press again on a visible line | Fresh start; held buttons never restart |

Digital classification: [line_barcode](app_program/line_barcode/README.md). No IR
black/white capture or calibration presses are needed. Both white means forward;
both black means junction and cannot start a run. Check DOUT wiring, black
polarity and comparator thresholds before physical use. Controls are local;
GP20 starts and GP21 stops/resets. If stopped, record the reason before reset.
Software state does not prove the wheels are stationary.

### Wireless telemetry

Use a reachable private 2.4 GHz LAN; the lab broker allows anonymous access without TLS, so keep it on the trusted bench network.

1. `test -e config/wifi_secrets.h || cp config/wifi_secrets.example.h config/wifi_secrets.h`; edit `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_BROKER_IP` (laptop LAN IPv4, not `127.0.0.1`/`0.0.0.0`); for authenticated brokers also `MQTT_USERNAME`, `MQTT_PASSWORD`. Client-ID prefix gets a radio-MAC suffix; older local headers may omit new settings. The file is git-ignored; do not commit or print it.

2. Start the broker and dashboard as in [Quick start](#quick-start) step 4.
3. Open <http://127.0.0.1:8080> or run the optional terminal monitor below.
4. Flash wireless TEST on USB alone and confirm messages originate from the Pico. After physical readiness, use normal firmware on battery alone and recalibrate.

Laptop tool (binary protocol, schema v2):

```sh
~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py monitor --broker 127.0.0.1
~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py map A B C D --broker 127.0.0.1
~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py snapshot --broker 127.0.0.1
```

Linux/WSL: install `mosquitto` and `mosquitto-clients`, allow Pico access through
firewall/WSL boundaries, and install `scripts/requirements-mqtt.txt` in a Python virtual environment.

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

## Bits per subsystem

Target sizes from `sizeof` with `arm-none-eabi-gcc` for the RP2040. **Used** is the bits the valid range needs; **stored** is what the data occupies, so the gap is padding or a whole word kept for a single atomic store. Ranges and reasons for each width: [data table](docs/project-specs.md#5-data-representation-table-week-13).

| Subsystem | Used bits | Stored bits |
| --- | ---: | ---: |
| [Comms](app_program/comms/README.md) | 1227 | 1352 |
| [Motion](app_program/motion/README.md) | 1137 | 1256 |
| [Line and barcode](app_program/line_barcode/README.md) | 2470 | 3560 |
| [Terrain](app_program/terrain/README.md) | 451 | 520 |
| [Obstacle](app_program/obstacle/README.md) | 508 | 680 |
| Mission | 1000 | 1136 |

Totals cover the rows below, multiplied by their instances. Private working structs are listed under the tables and not counted, because they mix packed fields with full-width times.

**Comms**

| Data | Fields (bits) | Instances | Used | Stored |
| --- | --- | ---: | ---: | ---: |
| Event ring entry | kind 3, arg 7, time 16, counts 2x16, arg2 16 | 16 | 74 | 80 |
| Event ring indices | head 4, count 5 | 1 | 9 | 16 |
| Event ring drop count | 32 | 1 | 32 | 32 |
| Event ring padding | none | 1 | 0 | 16 |
| Network fault byte | Wi-Fi, broker | 1 | 2 | 8 |

Messages on the wire are separate from these totals:

| Frame | Used | Stored |
| --- | ---: | ---: |
| Telemetry and snapshot reply | 183 | 184 |
| Event | 83 | 88 |
| Wheel trace | 120 | 120 |
| Line trace | 124 | 136 |
| Mapping | 36 | 40 |
| Junction test | 19 | 24 |
| Acknowledgement | 18 | 24 |
| Snapshot request | 16 | 16 |

**Motion**

| Data | Fields (bits) | Instances | Used | Stored |
| --- | --- | ---: | ---: | ---: |
| Encoder count | wrapped count 16 | 2 | 16 | 32 |
| Feedback word | status 2, action 6 | 1 | 8 | 32 |
| Command | time 32, target 12, speed 9, action 6, mode 2, steering 8 | 1 | 69 | 96 |
| Health | service time 32, sequence 32, stopped 1, brake ack 1 | 1 | 66 | 96 |
| Wheel trace record | see frame above | 8 | 120 | 120 |
| Motion fault byte | stall, encoder | 1 | 2 | 8 |

Not counted: `MotionController`, 480 stored.

**Line and barcode**

| Data | Fields (bits) | Instances | Used | Stored |
| --- | --- | ---: | ---: | ---: |
| IR raw word | left 1, right 1, valid 1 | 2 | 3 | 32 |
| Mapping word | letters 4x5, mapped 1 | 1 | 21 | 32 |
| Barcode edge | counts 2x16, level 1 | 64 | 33 | 48 |
| Barcode event | counts 2x16, letter 5, action 3, orientation 1 | 8 | 41 | 48 |
| Line fault byte | IR invalid, capture overflow, event overflow | 1 | 3 | 8 |

Not counted: `IrTracker`, 160 stored; `LineIrSnapshot`, 192 stored; `barcode_frame_t`, 496 stored.

**Terrain**

| Data | Fields (bits) | Instances | Used | Stored |
| --- | --- | ---: | ---: | ---: |
| Terrain word | last 7, max 7, count 8, state 3, height valid 1, calibrated 1 | 1 | 27 | 32 |
| `TerrainContext` | filter 3x32, calibration 3x23, rise 16, travel 16, roll average 32, pitch 15, roll 15, codes 2x7, count 8, flat run 4, calibration count 7, state 3, flags 2 | 1 | 297 | 320 |
| `TerrainSnapshot` | word 27, times 2x32, sequence 32, valid 1 | 1 | 124 | 160 |
| Terrain fault byte | IMU, calibration, impact | 1 | 3 | 8 |

**Obstacle**

| Data | Fields (bits) | Instances | Used | Stored |
| --- | --- | ---: | ---: | ---: |
| Echo word (front, side) | range 13, valid 1, time 16 | 2 | 30 | 32 |
| Scan word | state 3, scan ID 4 | 1 | 7 | 32 |
| Side-watch word | on 1, angle 6 | 1 | 7 | 32 |
| Scan ray | angle 6, valid 1, range 16 | 9 | 23 | 32 |
| Profile | angle 6, valid 1, measurements 6x16 | 1 | 103 | 112 |
| Range group | values 5x13, counters 4x3 | 1 | 77 | 96 |
| Profile evidence | widths 2x16, rays 3x4, edges 2 | 1 | 46 | 48 |
| Obstacle fault byte | no echo | 1 | 1 | 8 |

**Mission**

| Data | Fields (bits) | Instances | Used | Stored |
| --- | --- | ---: | ---: | ---: |
| Published state | state 3 | 1 | 3 | 8 |
| Safety latched cause | causes 4 | 1 | 4 | 32 |
| Line trace record | see frame above | 8 | 124 | 136 |
| Mission fault byte | line lost | 1 | 1 | 8 |

Not counted: `Mission`, 1792 stored, including `recovery_control_t` (128) and `bypass_control_t` (704).

## Manual build and flash

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
