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
- [MQTT binary protocol, schema version 3](#mqtt-binary-protocol-schema-version-3)
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

The periods are software settings. Measured execution times and stack use are recorded in [the team guide](team.md#timing).

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

Mission chooses STOP, FOLLOW, DISTANCE or TURN. Each command carries the source sample time. Discrete actions also carry an ID so Mission can match RUNNING, DONE or FAILED feedback to the command it issued. Motion owns the motor outputs, checks GP21 independently and brakes on expired commands. Reusing a sensor reading preserves its original timestamp. Each wheel encoder wraps at 16 bits. Motion, barcode, Terrain and bypass logic keep separate origins and subtract counts modulo 65,536 to measure travel.

### Shared-data rules

- A single writer publishes a small snapshot with a release store. Readers use an acquire load and unpack the fields from that copy.
- Larger snapshots, including the motion command and obstacle profile, use short mutex-protected copies. Sensor delays and network calls happen outside these locks.
- GPIO interrupts capture edges and counts. The Mission task decodes queued barcode edges. A bounded bridge passes complete MQTT frames between I/O and the kernel transport. Events from Mission and Terrain share a mutex because their producers run on different cores.
- I/O passes one mapping or junction-test request at a time to Mission and waits for its decision before acknowledging it. The kernel transport owns the MQTT connection. Local firmware retains movement authority.

### Fault policy

GP21, a lift, a Motion stall or an encoder fault can latch a stop. Missing required line data or stale Motion service also engages the safety checks. A missing sonar echo gives an unknown range, and Mission stops a manoeuvre when it lacks the evidence needed to continue. Wi-Fi and MQTT are prerequisites to start. A disconnection during healthy driving is reported and local control continues. The team guide maps each condition to its [owner and fault bit](team.md#fault-bitmask).

## MQTT binary protocol, schema version 3

Implemented by `app_program/comms/comms.h` and `mqtt_frame.c`, tested by `testing/comms/logs/S1-U05/test_mqtt_frame.c`; the laptop decoder `scripts/mqtt_client.py` checks the same golden vectors (`--self-test`). Field widths follow the [data table](team.md#6-data-representation). Version 3 packs the six owner fault bytes into one 12-bit fault word, so telemetry shrinks from 22 to 18 bytes; version 1 and 2 frames are rejected. A laptop decoder and firmware must be on the same version.

Topics (root `robot/s3`, set in `config/wifi_secrets.h`): `<root>/telemetry` carries every robot-to-PC frame, `<root>/request` carries mapping, junction-test and snapshot requests, `<root>/status` is the retained online/offline text.

Rules for every frame:

- Little-endian multi-byte fields, written with explicit masks and shifts, never by copying a struct.
- Byte 0: bits 7-4 version (3), bits 3-0 type.
- Length is exact for the type. A receiver rejects a wrong length, version or type, any set reserved bit and any out-of-range field. Nothing is partially accepted.
- Mission accepts mapping updates in READY or STOPPED. The junction-test request can queue one direction while following the line.
- Normal telemetry omits speed/direction: the laptop derives averages from
  successive counts. The separate wheel trace carries release-speed diagnostics.

| Type | Value | Direction | Length (bytes) |
| --- | --- | --- | --- |
| Telemetry | 1 | robot to PC, every 500 ms | 18 |
| Event | 2 | robot to PC | 11 |
| Snapshot reply | 3 | robot to PC, answers a request; body as telemetry | 18 |
| Mapping | 4 | PC to robot | 5 |
| Acknowledgement | 5 | robot to PC | 3 |
| Snapshot request | 6 | PC to robot | 2 |
| Junction test | 7 | PC to robot, queue a direction | 3 |
| Wheel trace | 8 | robot to PC, best effort per Motion release | 15 |
| Line trace | 9 | robot to PC, best effort per fresh IR acquisition while moving | 17 |
| Barcode trace | 10 | robot to PC, best effort, raw GP27 run widths | 18 |
| Terrain trace | 11 | robot to PC, best effort, IMU sample for the dashboard | 14 |

### Wheel trace (15 bytes)

Type 8 reports the wheel controller’s targets, measurements and applied duty.
Byte 0 is `38` hex; bytes 1..4 are robot release time uint32 ms. Bytes 5..8
contain left target in bits 0..14 and right target in bits 15..29 (signed
15-bit counts/s), TEST inhibition in bit 30 and motor-output service success
in bit 31. Bytes 9..10 and 11..12 are signed 16-bit measured left/right counts/s.
Bytes 13 and 14 are signed left/right duty percentages. Invalid encodings and
rounding are owned by the [data table](team.md#motion-and-encoder).
Output failure marks both duty fields invalid. TEST reports zero duty with
inhibition set. Targets represent computed wheel demands; serviced braking may
override them. This is software output evidence, not electrical measurement.

Motion encodes into an eight-record SPSC ring after output service and outside
its IRQ exclusion. I/O drains at most two per release, after normal reporting;
offline/full TX and full trace ring drop diagnostic samples without waiting.
Robot timestamp gaps reveal missing releases; timestamps wrap modulo 2^32.
There is no diagnostic request, movement authority or live tuning input.
Use the laptop decoder shipped with the firmware.

### Barcode trace (18 bytes)

Type 10 is an outbound barcode diagnostic. Byte 0 is `3A`
hex, byte 1 a wrapping sequence, bytes 2..17 eight little-endian runs: bit 15 black level, bits
14..0 width in encoder counts after chatter is merged, zero for an unused slot. A full ring drops frames.

### Terrain trace (14 bytes)

Type 11 is outbound and best effort, sent on every image for the dashboard's IMU graphs: 10 per second on flat ground, 20 on a hump. Byte 0 is `3B` hex, bytes
1..2 time ms low 16, byte 3 signed pitch in quarter degrees, byte 4 bits 0..2 state, 6 encoder valid, 7 calibrated,
byte 5 peak rise mm, byte 6 acceleration in 10 mg units, byte 7 signed roll in quarter degrees, bytes 8..10 signed X, Y and Z acceleration in 16 mg units (saturate at +/-2032 mg), byte 11 current height estimate mm, bytes 12..13 travel mm (wraps at 65536).

### Line trace (17 bytes)

Type 9 records the line-following decision for a fresh IR sample. Byte 0 is `39` hex; bytes 1..4 are the
IR acquisition time uint32 ms. Byte 5 flags: bit 0 left black, 1 right black,
2 in broad black, 3 junction candidate open, 4 junction qualified, 5 curved entry,
6 junction locked, 7 raw barcode GP27 level. Byte 6: bits 0..3 Mission state, 4..6 junction phase,
7 zero. Bytes 7..8 and 9..10 are the low 16 bits of the left/right encoder counts.
Byte 11 is the steering law output, signed Q7. Byte 12 is the law state: the side
last touched (-1 left, +1 right, 0 none) times the integral held at the last touch transition, signed Q7. Byte 13 is the FOLLOW steering sent to Motion (0 for
other modes). Bytes 14..15: bits 0..8 commanded speed mm/s, 9..10 motion mode,
11..15 running barcode edge count (wraps). Byte 16 is, while both sensors are black, the distance in mm between
the two sensors' black onsets (254 caps it), else 255.

Mission builds one record per fresh acquisition after `mission_step` while it was
moving (so the stopping decision is included) and puts it in an eight-record SPSC
ring; I/O drains at most two per release after the wheel trace, and offline/full
TX or a full ring drops records without waiting. Sample-time gaps reveal drops.
There is no request, movement authority or live tuning input. `mqtt_client.py
monitor` prints it as a short `LINE` line (`--raw` for JSON); the dashboard does not display it.

### Telemetry and snapshot reply (18 bytes)

| Byte | Bits | Field | Range |
| --- | --- | --- | --- |
| 0 | 7-0 | version and type | `0x31` telemetry, `0x33` reply |
| 1 | 7-0 | sequence | 0-255, wraps |
| 2 | 3-0 | mission state | 0-15 |
| 2 | 5-4 | motion status | 0 IDLE, 1 RUNNING, 2 DONE, 3 FAILED |
| 2 | 7-6 | line status | 0 unknown, 1 visible, 2 lost, 3 broad black |
| 3 | 5-0 | motion action ID | 0-63 |
| 3 | 6 | mapped | four letters installed |
| 3 | 7 | reserved | 0 |
| 4-5 | 1-0 | Motion faults: stall, encoder | [fault bitmask](team.md#fault-bitmask) |
| 4-5 | 4-2 | Line faults: IR invalid, capture overflow, event overflow | |
| 4-5 | 7-5 | Terrain faults: IMU, calibration failed, lifted | |
| 4-5 | 8 | Obstacle fault: no echo | |
| 4-5 | 9 | Mission fault: line lost | |
| 4-5 | 11-10 | Network faults: Wi-Fi lost, broker lost | |
| 4-5 | 15-12 | reserved | zero; a set bit is rejected |
| 6-7 | 15-0 | left encoder count | wrapped, `ENC_COUNT_BITS` wide |
| 8-9 | 15-0 | right encoder count | wrapped |
| 10 | 7-0 | hump count | 0-255 saturating |
| 11 | 6-0 | maximum hump height code | 7-bit result, 1 mm per count |
| 11 | 7 | maximum valid | set once any hump completed |
| 12-13 | 12-0 | front range, mm | 0-8190; 8191 invalid |
| 12-13 | 13 | front range valid | no echo is not clear space |
| 12-13 | 15-14 | IMU calibration | 0 unknown, 1 not calibrated, 2 calibrated, 3 invalid |
| 14 | 4-0 | last decoded barcode letter, cleared when its navigation ends on the line | 0-25 (A-Z), 31 none |
| 14 | 7-5 | action it maps to | 0 unmapped, 1 left, 2 right, 3 straight, 4 U-turn |
| 15-17 | 9-0 | latest obstacle width, mm | 0 none; saturates at 1023 |
| 15-17 | 19-10 | clearance on the side chosen for the latest bypass, mm | 0 none; saturates at 1023 |
| 17 | 7-4 | reserved | zero; a set bit is rejected |

The fault word packs each owner's defined bits in owner order. The firmware
keeps one atomic byte per owner, because each owner is that byte's only
writer; the encoder only reads the six bytes, so packing them on the wire
needs no lock. Undefined owner bits are never sent.

IMU calibration reports Terrain's calibration flag independently of hump results
and current sensor faults.

### Event (11 bytes)

| Byte | Field |
| --- | --- |
| 0 | version and type, `0x32` |
| 1 | bits 3-0 kind: 1 barcode, 2 hump, 3 obstacle, 4 turn; bits 7-4 reserved 0 |
| 2 | arg: letter (barcode), height code (hump), action (turn) |
| 3-4 | time, low 16 bits of kernel milliseconds |
| 5-6 | left encoder count at the event |
| 7-8 | right encoder count at the event |
| 9-10 | arg2: action (barcode), clearance mm (obstacle); arg 1 passing left, 0 right |

**Event RAM packing.** The 11-byte event wire format is unchanged. The
Reporting ring holds 16 `comms_packed_event_t` entries of 10 bytes: kind (3 bits)
and arg (7 bits) share a 16-bit field; time, encoder positions and arg2 stay
16-bit. Ring head (0-15) and count (0-16) share one 16-bit word, 162 bytes in
all. Invalid kinds and arguments above 127 are rejected rather than truncated.
A full ring overwrites the oldest event, and a lock that cannot be taken within
its bound drops the new one; neither drop is counted.

### Mapping, acknowledgement, snapshot request

- **Mapping (5 bytes):** byte 0 `0x34`, byte 1 sequence, bytes 2-4 a 24-bit little-endian value; bits 4-0, 9-5, 14-10 and 19-15 are the letters for left, right, straight and U-turn (A=0 .. Z=25); bits 23-20 reserved 0. The four letters are distinct.
- **Acknowledgement (3 bytes):** byte 0 `0x35`, byte 1 the sequence answered, byte 2 the result: 0 OK, 1 busy, 2 invalid, 3 malformed. Mission returns the outcome of a mapping or direction request.
- **Snapshot request (2 bytes):** byte 0 `0x36`, byte 1 sequence.
- **Junction test (3 bytes).** Send `37 sequence action` in hexadecimal layout, where action is 1 left, 2 right, 3 straight or 4 U-turn. An OK acknowledgement means Mission queued the instruction.

### Golden vectors

Used by the host tests and `scripts/mqtt_client.py --self-test`.

| Frame | Bytes |
| --- | --- |
| Telemetry (faults: Motion stall, Line capture overflow, Terrain lifted, no echo, line lost, both Network) | `31 2A 72 45 89 0F 34 12 78 56 03 F8 D0 30 61 A4 A9 02` |
| Event (barcode Z, action 2) | `32 01 19 E8 03 10 00 11 00 02 00` |
| Mapping A left, B right, C straight, D U-turn, sequence 7 | `34 07 20 88 01` |
| Acknowledgement installed, sequence 7 | `35 07 00` |
| Snapshot request, sequence 9 | `36 09` |

## Tests and measurements

Run the host suite from the repository root with `sh testing/tools/run.sh`. For one test, append its ID, for example `sh testing/tools/run.sh S2-U05`. With the Arm toolchain installed, `sh testing/tools/sizeof_probe.sh` reports the target structure sizes, and `sh testing/tools/build_pending_profiles.sh` builds the firmware matrix.

The team recorded the full-course SYS-U01 pass on trial firmware `880ff16` on 2026-11-20. The system log identifies the two runs, the recorded measurements and the stop condition for each. [Testing guide](testing/README.md) · [System result](testing/system/TEST_LOG.md#results) · [Video index](video/README.md#video-index)
