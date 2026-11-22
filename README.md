# CSC01 Autonomous Robotic Car

## Contents

- [Codebase guide](#codebase-guide)
- [Install dependencies on Ubuntu](#install-dependencies-on-ubuntu)
- [Configure the robot and broker](#configure-the-robot-and-broker)
- [Build profiles](#build-profiles)
- [Power and flashing](#power-and-flashing)
- [Start a course run](#start-a-course-run)
- [Architecture](#architecture)
  - [Tasks](#tasks)
  - [Mission loop](#mission-loop)
  - [Motion command contract](#motion-command-contract)
  - [Shared-data rules](#shared-data-rules)
  - [Fault policy](#fault-policy)
- [MQTT topics](#mqtt-topics)
- [Tests and measurements](#tests-and-measurements)

The robot runs micro T-Kernel 3.0 on a Raspberry Pi Pico WH and a Robo Pico carrier. It follows a line, reads Code 39 barcodes to choose junction turns, measures humps, and scans obstacles before driving around them. A laptop receives telemetry over Wi-Fi. The local buttons start and stop the car.

## Codebase guide

| Start here | What it does |
| --- | --- |
| [Application entry point](app_program/app_main.c) and [configuration](app_program/common/robot_config.h) | Initialize subsystems, start tasks, set pin assignments, periods, thresholds and controller gains. |
| [Mission](app_program/mission/) | Combine sensor evidence, choose a motion command, handle stop/reset, and drive the GP18 state LEDs. |
| [Motion](app_program/motion/README.md) | Read quadrature encoders, close the wheel-speed loops, and apply PWM or braking. |
| [Line and barcode](app_program/line_barcode/README.md) | Sample the two digital line sensors and decode GP27 edges using encoder travel. |
| [Obstacle](app_program/obstacle/README.md) | Position the servo, time sonar echoes, and estimate obstacle width and clearance. |
| [Terrain](app_program/terrain/README.md) | Calibrate the accelerometer at boot, estimate hump height, and detect a lift. |
| [Comms](app_program/comms/README.md) and [laptop tools](scripts/) | Encode reports, bridge requests to Mission, and display telemetry. |
| [Kernel](kernel/), [drivers](device/), [libraries](lib/) and [build](build_make/) | Supply the scheduler, hardware support, network stack integration, and firmware Makefiles. |
| [Team and hardware](team.md) | Find subsystem owners, the GPIO wiring table, packed data fields, and memory measurements. |

## Install dependencies on Ubuntu

Use Ubuntu 24.04 on a PC, or Ubuntu 24.04 under WSL2 for compilation. Run the commands below from the repository root. The host C++ compiler builds the UF2 converter. The Arm compiler and newlib build the Pico firmware. Python runs the laptop tools, and Node.js runs the dashboard tests.

```sh
sudo apt update
sudo apt install build-essential git gcc-arm-none-eabi libnewlib-arm-none-eabi \
    python3 python3-venv mosquitto nodejs screen
mkdir -p "$HOME/pico"
git clone --branch 2.2.0 --depth 1 https://github.com/raspberrypi/pico-sdk.git "$HOME/pico/pico-sdk"
export PICO_SDK_PATH="$HOME/pico/pico-sdk"
git -C "$PICO_SDK_PATH" submodule update --init --depth 1 lib/tinyusb lib/cyw43-driver lib/lwip
arm-none-eabi-gcc --version

python3 -m venv "$HOME/.robot-car-tools/venv"
"$HOME/.robot-car-tools/venv/bin/python" -m pip install -r scripts/requirements-mqtt.txt
```

If the SDK is already installed, point `PICO_SDK_PATH` at its 2.2.0 checkout and initialize the three submodules there. Use a path without spaces. Set the variable in each build terminal or add the export to `~/.bashrc`.

## Configure the robot and broker

1. Create the local header, preserving any existing settings.

   ```sh
   test -e config/wifi_secrets.h || cp config/wifi_secrets.example.h config/wifi_secrets.h
   ```

2. Edit `config/wifi_secrets.h`. Set `WIFI_SSID` and `WIFI_PASSWORD` for the private 2.4 GHz network. Set `MQTT_BROKER_IP` to a broker address reachable **from the Pico**. On native Linux, `ip -4 -brief address` lists the laptop's interfaces. A broker inside WSL2 needs a reachable listener and forwarding or mirrored networking. Running the broker on the Windows host also works. Rebuild the firmware after editing the header.
3. Start one broker on port 1883. The supplied configuration listens on all IPv4 interfaces and allows anonymous access, so use it on the trusted lab network.

   ```sh
   mosquitto -c config/mosquitto-lab.conf
   ```

   If the Ubuntu package already started a listener, inspect it with `ss -ltn 'sport = :1883'`. Use that broker if it accepts connections from the Pico, or stop the default service before starting the lab listener.

## Build profiles

Build from the repository root. For a first USB check, this complete command produces `build_make/test.uf2` with motor outputs inhibited.

```sh
make -C build_make -j"$(nproc)" SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 \
    CRASH_REPORT=0 NOGUARDS=0 TEST=1 FULL_COURSE=0 TRACE=1 BENCH=0 EXE_FILE=test
```

Keep the common options through `NOGUARDS=0`. Replace the five options starting with `TEST` with the row for the image you need. Run one build at a time because the profiles share object files. Each row produces `build_make/<EXE_FILE>.uf2`.

| Image | Options | Use |
| --- | --- | --- |
| test | `TEST=1 FULL_COURSE=0 TRACE=1 BENCH=0 EXE_FILE=test` | Boot self-tests over USB. GP20 requests a stationary sonar scan. |
| full_course | `TEST=0 FULL_COURSE=1 TRACE=0 BENCH=0 EXE_FILE=full_course` | Course driving with junction turns, obstacle bypass and stall detection. |
| trial | `TEST=0 FULL_COURSE=1 TRACE=1 BENCH=0 EXE_FILE=trial` | Course driving with wheel, line, barcode and task timing diagnostics. |
| bench | `TEST=0 FULL_COURSE=1 TRACE=1 BENCH=1 EXE_FILE=bench` | A GP20 press runs the next distance or pivot in the motion sequence. |
| bench10 | `TEST=0 FULL_COURSE=1 TRACE=1 BENCH=2 EXE_FILE=bench10` | Each press runs one 100 mm straight. |
| benchturn | `TEST=0 FULL_COURSE=1 TRACE=1 BENCH=3 EXE_FILE=benchturn` | Each press runs the next left, right or 180° pivot. |

Some line and terrain tests require an extra build option. Append `BARCODE_DIAGNOSTICS=1` or `TERRAIN_DIAGNOSTICS=1` to the selected make command when the runbook calls for it.

USB CDC and `CRASH_REPORT=0` leave GP0 and GP1 available to the right encoder. The course image still sends ordinary telemetry, events and terrain samples. Build settings are defined in the [RP2040 Makefile](build_make/pico_rp2040.mk).

## Power and flashing

Use one power source at a time. Disconnect battery power before attaching USB. For a driving run, unplug USB before connecting the battery.

1. Hold **BOOTSEL** on the Pico while connecting USB. Release it when the **RPI-RP2** storage volume appears.
2. Copy the UF2 for the chosen profile onto that volume. On Ubuntu, check the mount point with `lsblk -o LABEL,MOUNTPOINTS`. For the test image, a typical desktop mount is shown below. On WSL2, open `build_make` with `explorer.exe build_make` and copy the UF2 to **RPI-RP2** in Windows Explorer.

   ```sh
   cp build_make/test.uf2 "/media/$USER/RPI-RP2/"
   ```

3. The Pico reboots and the storage volume disappears. For USB self-tests, open the board's serial device, usually `screen /dev/ttyACM0 115200` on Linux. Look for the subsystem test results and summaries. To identify a saved image later, compute its checksum with `sha256sum build_make/test.uf2`.
4. To drive, repeat with `trial.uf2` or `full_course.uf2`, then disconnect USB and boot on battery on a level surface. Calibration and barcode mapping start afresh after each reboot.

## Start a course run

1. Keep the car stationary during boot calibration. Join the laptop and Pico to the configured network. Start the dashboard on the broker host in a separate terminal.

   ```sh
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py serve --broker 127.0.0.1
   ```

2. Open [the local dashboard](http://127.0.0.1:8080). In READY or STOPPED, map four distinct letters to **left, right, straight and U-turn**. Wait for the acknowledgement. To send the same mapping from a terminal, run

   ```sh
   ~/.robot-car-tools/venv/bin/python scripts/mqtt_client.py map A B C D --broker 127.0.0.1
   ```

3. Position the line sensors on the course. From READY, release and freshly press **GP20**. Mission checks mapping, connectivity, fresh sensor and Motion data, and start-blocking faults.
4. Press **GP21** to stop. Clear the cause of a fault stop, then press and release GP21 after braking has been serviced. This returns the robot to READY. A separate GP20 press starts another run.

The laptop client also supports `monitor` for live records and `snapshot` for a single report. If telemetry is absent, check the broker's LAN address in the flashed image, port 1883, and whether wireless clients can reach each other.

## Architecture

### Tasks

The application starts three periodic tasks. Motion and Mission share the core that captures encoder, barcode and echo edges. The other core runs sensor reporting alongside the kernel's Wi-Fi and USB services. Lower priority numbers run first.

| Task | Core | Priority | Configured period | Work per release |
| --- | ---: | ---: | ---: | --- |
| Motion | 1 | 13 | 20 ms | Check local stop and command age, read wheel counts, update PID, apply output and publish feedback. |
| Mission | 1 | 14 | 10 ms | Step the sensors and safety logic, accept eligible requests, advance the mission and issue one motion command. |
| I/O | 0 | 16 | 10 ms | Step Terrain acquisition and reporting. Service bounded numbers of requests and events. |

The firmware sets these priorities and periods in [robot_config.h](app_program/common/robot_config.h).

### Mission loop

| State | What the robot does |
| --- | --- |
| READY | Holds still while waiting for a local start and valid inputs. |
| FOLLOW_LINE | Steers from the two digital line sensors. A decoded barcode supplies an action for the next junction. |
| NAVIGATE | Uses line evidence and encoder travel to execute the junction action, then returns to tracking. |
| SCAN | Brakes, confirms wheel stationarity, then sweeps the sonar to estimate obstacle width and free space. |
| BYPASS | Takes a side supported by the scan, watches the obstacle flank and finds the original line again. |
| REACQUIRE | Searches for a lost line with bounded encoder-controlled sweeps. Exhausting the search stops the car. |
| STOPPED | Keeps braking latched until the local reset conditions are met. |

The two line sensors straddle the stripe. One black sensor gives Mission a steering correction. Both white can mean the stripe is centred between the sensors, so Mission measures wheel travel since the last contact before declaring a loss. At a qualified crossing, Mission consumes the held barcode action. For a turn it advances the axle towards the crossing centre, pivots and uses fresh readings from the leading sensor to find the outgoing branch. Encoder angle and action deadlines bound the search. [Mission state machine](app_program/mission/mission_logic.c)

Terrain runs alongside these states. It combines accelerometer tilt and wheel travel to estimate hump height and reports a lift to the safety logic. The dashboard can submit a junction-test direction during eligible line following. Mission decides whether to accept it and uses it once at a junction.

### Motion command contract

Mission chooses STOP, FOLLOW, DISTANCE or TURN. Each command carries the source sample time. Discrete actions also carry an ID so Mission can match RUNNING, DONE or FAILED feedback to the command it issued. Motion owns the motor outputs, checks GP21 independently and brakes on expired commands. Reusing a sensor reading preserves its original timestamp. Motion, barcode, Terrain and bypass logic each measure wheel travel from their own encoder starting counts.

### Shared-data rules

- A single writer publishes a small snapshot with a release store. Readers use an acquire load and unpack the fields from that copy.
- Larger snapshots, including the motion command and obstacle profile, use short mutex-protected copies. Sensor delays and network calls happen outside these locks.
- GPIO interrupts capture edges and counts. The Mission task decodes queued barcode edges. A bounded bridge passes complete MQTT frames between I/O and the kernel transport. Events from Mission and Terrain share a mutex because their producers run on different cores.
- I/O passes one mapping or junction-test request at a time to Mission and waits for its decision before acknowledging it. The kernel transport owns the MQTT connection. Local firmware retains movement authority.

### Fault policy

GP21, a lift, a Motion stall or an encoder fault can latch a stop. Missing required line data or stale Motion service also engages the safety checks. A missing sonar echo gives an unknown range, and Mission stops a manoeuvre when it lacks the evidence needed to continue. Wi-Fi and MQTT are prerequisites to start. A disconnection during healthy driving is reported and local control continues. The team guide maps each condition to its [owner and fault bit](team.md#fault-bitmask).

## MQTT topics

The laptop reads telemetry, events and snapshot replies from `robot/s3/telemetry`. It sends mapping, junction-test and snapshot requests to `robot/s3/request`. The retained `robot/s3/status` topic reports whether the robot is online. The firmware topic names are set in `config/wifi_secrets.h`. For field widths, byte order and example payloads, see the [MQTT frame reference](team.md#mqtt-binary-protocol-schema-version-3).

## Tests and measurements

Run the host suite from the repository root with `sh testing/tools/run.sh`. For one test, append its ID, for example `sh testing/tools/run.sh S2-U05`. With the Arm toolchain installed, `sh testing/tools/sizeof_probe.sh` reports the target structure sizes, and `sh testing/tools/build_pending_profiles.sh` builds the firmware matrix.

The team recorded the full-course SYS-U01 pass on trial firmware `880ff16` on 2026-11-20. The system log identifies the two runs, the recorded measurements and the stop condition for each. [Testing guide](testing/README.md) · [System result](testing/system/TEST_LOG.md#results) · [Video index](video/README.md#video-index)
