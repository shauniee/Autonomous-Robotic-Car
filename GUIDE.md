# Operating the robot

The robot drives itself. You power it, calibrate it, load four barcode assignments over MQTT, then start it locally. The laptop displays readings and supplies configuration; it does not drive the wheels.

The software is integrated on `uat`. Measured configuration and physical acceptance are still required. Do not treat compilation or a confirmation flag as a successful hardware trial. [Current results](testing/integration/TEST_LOG.md) own the exact validation level.

## Buttons and sounds

| Control | Meaning |
| --- | --- |
| Pico BOOTSEL | Install firmware; not the mission start button |
| GP20 | First accepted calibration press: black + IMU. Next: white. When READY: start. After stop: reset, then a separate start |
| GP21 | Independent stop; cancels an unfinished calibration or scan |
| One beep | Both IR black and IMU calibration succeeded |
| Two beeps | IR white/contrast accepted |
| Three beeps | Calibration failed or was cancelled; inspect the reported result |

Release GP20 before each new press. While combined calibration is running, extra presses are ignored rather than queued. After reset, wait for READY and release/press again to start.

## One-time preparation

Use the pin/resource map in [TEAM](TEAM.md) and electrical facts in [TECHNICAL_INFO](TECHNICAL_INFO.md). Establish actual motor/encoder polarity, counts/geometry, PID/tolerances, IMU mounting/reference model, sonar mounting offsets, servo endpoints, worst-case braking headroom and course bounds. Populate `app_program/common/robot_config.c` only from recorded measurements. Confirmation flags default false and deliberately inhibit movement.

The pending-decision register is [PROJECT_SPECS](docs/donotsubmit/PROJECT_SPECS.md#pending-decisions). Confirm issued barcode format, junction spacing, finish/action-count convention, obstacle-depth bounds and WiFi/broker access before program freeze. Current runtime configuration changes barcode assignments only; other settings are compiled.

## Build profiles

Commands run from the repository's `code/` folder. Install Make, the Arm bare-metal compiler and a complete Pico SDK; set `PICO_SDK_PATH` to its actual path without spaces. Build profiles sequentially because they share generated files.

```sh
# USB-only inspection: motors inhibited
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=1
# Offline normal
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=0
# Wireless TEST: motors inhibited
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=1
# Normal wireless mission
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=0
```

| Profile | UF2 file in `build_make/` |
| --- | --- |
| Offline TEST | `mtk3pico_smp1_usb_cdc_wifi_test.uf2` |
| Offline normal | `mtk3pico_smp1_usb_cdc_wifi.uf2` |
| Wireless TEST | `mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_test.uf2` |
| Wireless normal | `mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt.uf2` |

Give every flag on each build. USB CDC and disabled UART crash reporting avoid encoder pin conflicts. TEST can run the controller internally but always brakes motor outputs.

## Power and flashing

**One power source at a time. USB alone for flashing/inhibited TEST; unplug USB before battery/power-bank operation.** Power changes reboot the robot, clearing calibration and RAM mappings.

1. Disconnect the battery. Hold BOOTSEL while connecting USB; release when RPI-RP2 appears.
2. Copy the correct UF2 onto that drive. The Pico restarts automatically.
3. For TEST, open exactly one serial monitor on the actual Pico USB port at 115200 baud. Release GP20, then press/release to execute fixtures. This diagnostic press precedes the normal calibration sequence.
4. Save all `[TEST] ID PASS|FAIL` output. Services then provide raw IR, encoder, IMU and Echo observations; invalid/unconfigured values are not accepted measurements.
5. After electrical release, flash reviewed normal firmware using USB alone. Unplug USB, use battery alone, and begin with raised wheels. Use wireless/camera/scope evidence during powered movement.

Measure commanded-brake timing separately from physical wheel stopping. Do not use the carrier's independent motor-test buttons as software stop evidence.

## Laptop and MQTT setup

Use a reachable configured 2.4 GHz network. Before freezing, populate ignored `config/wifi_secrets.h` from its example if absent: WiFi credentials, actual broker LAN IPv4 address/port and any required broker authentication. Never commit or print private credentials. `127.0.0.1` is not the laptop address the robot should use.

For the project's private lab broker, install Mosquitto and start it from `code/`:

```sh
mosquitto -c config/mosquitto-lab.conf
```

Use the instructor's agreed broker instead when required. The lab configuration is anonymous/no TLS and belongs on a trusted network. Mapping messages must not be retained; clear any old retained configuration before acceptance.

Install the helper dependency once, then monitor:

```sh
python3 -m pip install -r scripts/requirements-mqtt.txt
python3 scripts/mqtt_client.py monitor --broker <broker-ip>
```

Replace `<broker-ip>` with the actual configured address. Add `--port` / `--root` for configured alternatives. Laptop authentication uses `ROBOT_MQTT_USERNAME` and `ROBOT_MQTT_PASSWORD` environment variables.

The terminal monitor prints live text/JSON. Telemetry is nominally every 500 ms and heartbeat every second. The helper marks retained messages and reports missing heartbeat after three seconds. Networking does not authorize movement.

### Browser dashboard

With Mosquitto running on this Mac, start the website from `code/`:

```sh
python3 scripts/mqtt_dashboard.py
```

Open [the robot monitor](http://127.0.0.1:8080) in your browser. Keep its terminal open; Ctrl+C closes it. It uses the same MQTT dependency installed above. If another program uses port 8080, add `--web-port 8081` and open the printed address.

The default broker address `127.0.0.1` means Mosquitto on this Mac. The Pico still connects to the Mac's actual LAN address configured in its firmware. If your broker is on another computer, start with `python3 scripts/mqtt_dashboard.py --broker <broker-ip>`. Topic-root/port alternatives use `--root` and `--port`, matching the terminal helper. Authentication uses the same environment variables.

Expect mission state, wheel-speed traces, line sensor fractions/raw values, IMU calibration/tilt, sonar clearance, last scan, highest accepted hump and barcode encounters. The page updates automatically and has no movement or configuration controls. No firmware change is needed to use it with the existing schema-3 telemetry.

Broker connected and Robot live are separate indicators. Retained presence cannot make a robot live; fresh nonretained snapshots and heartbeat are required. The page displays the latest reported values between MQTT updates, respects each payload's validity/acquisition age, and hides current readings when the live feed expires after three seconds. Accepted run maximum and last scan remain labelled historical results. A dash means unavailable, not zero. Reloading the page clears its local speed/encounter history; producer counts come from the robot.

A TEST image can show `CALIBRATING` / configuration pending with blank uncalibrated readings. This confirms data delivery, not completed sensor calibration or mission readiness. [Dashboard checks](testing/subsystem1/TEST_LOG.md#dashboard-checks) distinguish synthetic broker/browser trials from observed robot-topic data.

## Every power-on: calibrate, map, start

1. Power the prepared robot, leave GP20/GP21 released, and keep it level and still. Motors stay braked. IMU bias calibration does not start automatically.
2. Put both mounted line sensors over a black reference patch at their normal height. Press/release GP20. Keep still until **one beep**: black capture and 100 fresh stationary IMU samples both succeeded, normally around two seconds. A five-second deadline bounds the operation.
3. Move the reference surface to white under both sensors. Press/release GP20. Keep still until **two beeps**, confirming adequate contrast. If you hear three, inspect the calibration status; a failed combined attempt requires a fresh black/IMU capture. Low white contrast clears IR references, so begin at black again.
4. Load the four assignments using the procedure below while stopped. Do not start after timeout or rejection based on an assumed new table.
5. Wait for telemetry `mission.state` to be `READY`. Missing settings/calibration/required fresh sensors prevent readiness. Combined IMU status is 0 idle, 1 running, 2 done, 3 failed, 4 cancelled.
6. Position on the starting line with valid line guidance. Clear the area, then press/release GP20. Movement can begin on the press; `FOLLOW_LINE` identifies the running mission.

GP21 during combined calibration cancels both and latches stop. Reset is a separate local action. Ordinary stopping retains completed calibration; an IMU reinitialization after fault requires a new combined capture. Reboot always requires recalibration and mapping reload.

Three beeps can result from the IMU half of the combined attempt failing even when the IR sensors are correctly over black. With the IMU disconnected, this build cannot complete calibration. Ultrasonic sensing is not part of that calibration step, but valid forward sensing is required before full mission start. Acquisition faults require stopped reinitialization and a fresh combined calibration; accepted run results remain available.

## Four-letter mapping without changing code

The decoder supports A-Z. Exactly four distinct letters are assigned, one each for left/right/straight/U-turn. The other 22 have no navigation command.

Create plain-text `mission-mapping.json`; this example is rehearsal data, not the official assignment:

```json
{"H":"left","Q":"right","M":"straight","Z":"u_turn"}
```

Validate locally and then send it while the robot is braked and stationary:

```sh
python3 scripts/mqtt_client.py mapping --validate-only --file mission-mapping.json
python3 scripts/mqtt_client.py mapping --broker <broker-ip> --file mission-mapping.json
```

Editing the file alone does nothing to the Pico. The helper reads it, expands a complete 26-entry table and sends an MQTT `barcode_mapping` request to `/mapping`. The robot applies it in RAM and replies on `/mapping_status`; no rebuild/reflash is involved.

Expected: matching request ID, `result:2` and an applied revision. Busy/invalid/no acknowledgement means do not assume the new table is installed. Configuration never starts movement. After reboot reload it. [Subsystem 1](docs/architecture/SUBSYSTEM1.md#mqtt-contract) owns the actual wire format.

## During the run

| Observation | Expected accepted behavior |
| --- | --- |
| Line/curves | Follows with calibrated guidance and encoder wheel control |
| Assigned barcode | Retains its instruction and acts at the associated confirmed junction |
| Unassigned letter | Reports the decode; produces no navigation command |
| Hump | Records accepted height, encounter and greatest valid run peak |
| Obstacle | Brakes early, scans only while stationary, restores forward sensing, then uses a supported bypass |
| Original line | Resumes only with expected pose/heading/corridor and fresh line observations |
| Unknown/blocked/ambiguous situation | Stops rather than guessing |
| Confirmed finish | Brakes and reports `COMPLETE` |

The supported finish is a confirmed sustained-black marker after configured action count and minimum travel. Hidden obstacle depth and original-route identity need course confirmation; odometry and sparse sonar do not prove every possible layout.

## Stop, reset and restart

Press GP21 to latch stop. Releasing it does not resume movement. Clear the cause, release GP21, press/release GP20 once for reset, wait for READY, then release/press again to start. Held inputs cannot restart. A fresh run clears run height/count/history; stop-only reset preserves accepted results.

| State/reason | What to do |
| --- | --- |
| `CALIBRATING` / configuration pending | Complete calibration/mapping and inspect unset measured/course settings |
| `FAULT` / required sensor | Inspect fresh/valid producer readings and wiring before reset |
| `FAULT` / barcode or ambiguous route | Inspect capture faults, pending commands and junction context |
| `FAULT` / blocked path | Change the test layout or confirm supported course bounds; do not force a bypass |
| Mapping busy/invalid/timeout | Keep braked; inspect the input, current revision and connection, then retry with the helper |

## Read status and encounters

```sh
python3 scripts/mqtt_client.py status --broker <broker-ip>
python3 scripts/mqtt_client.py events --broker <broker-ip> --kind hump --after-id 0
python3 scripts/mqtt_client.py events --broker <broker-ip> --kind barcode --after-id 0
```

Status is a fresh bounded robot snapshot. Event history holds the last eight accepted records per kind. Counts survive missed publications; `dropped` reports evicted records and `overflow` means a counter saturated. Event IDs start again with a new run; reset the cursor when the response's run changes. This is a bounded record, not permanent storage.

## Acceptance and submission

Use the five passes in [testing](testing/README.md). Required cases include all 26 letters at both orientations/scales, four actions, one-wheel reference humps, both bypass sides, stop/reset and failure recovery. Physical outcomes stay PENDING until measured. Agreement on numerical accuracy/clearance/time thresholds precedes PASS.

- [ ] Obtain owner review and fill the single pending-decision register from measurements/course answers.
- [ ] Execute inhibited fixtures and raised-wheel calibration; disable commissioning afterward.
- [ ] Complete feature/fault passes and repeated whole-course runs without intervention.
- [ ] Finalize compiled network/course settings before freeze; record the normal wireless UF2 checksum.
- [ ] Rehearse two different four-letter mappings and reboot/reload on identical firmware.
- [ ] Prepare required Motion tuning/accuracy and Terrain analysis reports from actual evidence.
- [ ] Confirm submission artifacts, instructor access and demonstration schedule; development references under `docs/donotsubmit/` are excluded from submission packaging.

On mission day change only the four assignments in the laptop data file, verify the live applied acknowledgement/revision, then start locally. Keep the frozen firmware unchanged. Save real results and video links under [testing](testing/README.md); the [video guide](videos/README.md) is preserved unchanged.

### Remaining release gates, excluding video

The current branch is software-integrated; it is not a certified completed mission. [The readiness audit](testing/integration/logs/2026-10-15_mission-readiness.json) records the current evidence. Complete each gate before declaring mission/submission readiness:

| Gate | What to do | Expected result |
| --- | --- | --- |
| Measured configuration | Resolve D03-D08 in the [pending register](docs/donotsubmit/PROJECT_SPECS.md#pending-decisions); enter recorded values, not confirmation flags alone | Motion/sonar/terrain/electrical configuration validates against real installed hardware |
| Course rules/freeze | Resolve D01/D02/D09/D10, including issued barcode format, junction/finish and supported bypass depth/boundaries | The final image supports the actual course before the four letters are announced |
| Startup and controls | Execute the inhibited fixtures, T01/T02 and raised-wheel measurements | Recorded fixture results, successful black+IMU then white calibration, independent brake and separate reset/start |
| Navigation and sensing | Execute T03-T11, including all alphabet/scales/orientations, four motions, reference/one-wheel humps and both bypass sides | Agreed accuracy/clearance criteria met; original route confirmed; uncertainty stops |
| Failure handling and course | Execute T12-T16, including stale/encoder/deadline/network/load faults and repeated full-course runs | Correct fault/reset behavior, measured timing/stack margin and complete autonomous runs on the frozen image |
| Submission evidence | Save configuration/image identities, complete Motion tuning/accuracy and Terrain analysis from measurements, and confirm instructor access/artifacts | Required reports and traceable acceptance records exist; video can be handled separately |

Missing sensors or unconfirmed measurements remain unavailable/pending. Successful compiler checks and a live MQTT dashboard do not replace these gates.
