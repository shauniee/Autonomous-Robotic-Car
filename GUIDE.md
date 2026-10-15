# Operating the robot

The robot drives itself. You power it, calibrate it, load four barcode assignments over MQTT, then start it locally. The laptop displays readings and supplies configuration; it does not drive the wheels.

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

Use the pin/resource map in [TEAM](docs/TEAM.md) and electrical facts in [TECHNICAL_INFO](docs/donotsubmit/TECHNICAL_INFO.md). Establish actual motor/encoder polarity, counts/geometry, PID/tolerances, IMU mounting/reference model, sonar mounting offsets, servo endpoints, worst-case braking headroom and course bounds. Populate `app_program/common/robot_config.c` only from recorded measurements. Confirmation flags default false and deliberately inhibit movement.

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

## Checks while IMU and ultrasonic are unavailable

Use the **IR-only TEST image** for these checks. It always inhibits motor
outputs, skips IMU calibration and cannot enter mission READY or powered
commissioning. Missing terrain/obstacle capability remains unavailable.

```sh
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=1 IR_ONLY_TEST=1
```

Its image is `build_make/mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_test_ir_only.uf2`.
For offline inspection use `NET=none MQTT=0`; the name then omits `_lwip_mqtt`.
`IR_ONLY_TEST=1 TEST=0` is rejected. Ordinary builds default to
`IR_ONLY_TEST=0` and retain combined black+IMU mission calibration.

1. Follow USB-only flashing above. At startup release GP20, then press/release
   it once to execute the firmware checks. Capture every PASS/FAIL between the
   run-start and run-end markers. Synthetic S4/S5 fixtures do not test missing
   physical sensors.
2. After services start, put both line sensors over black and press/release
   GP20: **one beep means IR black only in this image**. Put both over white
   and press/release again: two beeps confirm contrast. IMU stays uncalibrated.
3. Without more button presses, move black under left only, right only, both,
   then neither. Expect visible/visible/broad-black/lost readings. Keep mounting
   height unchanged. Actual tracking still needs a powered course trial.
4. Inspect raw encoder counts with wheels raised. Use a marked tyre and count
   full, slow forward turns while the other wheel stays still. Never force a
   stiff wheel. Record start/end counts and require zero faults. Uncertain turn
   counts or invalid edges cannot establish calibration.

```sh
python3 testing/capture_console.py --port /dev/cu.usbmodemMTK3PICO1 --seconds 90 --image build_make/mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_test_ir_only.uf2
# After fixtures, capture sensor observations without grading another test run:
python3 testing/capture_console.py --port /dev/cu.usbmodemMTK3PICO1 --seconds 120 --observe
```

Use the actual port and one reader. The standard-library helper sends no serial
commands and saves new raw output/metadata under `testing/integration/logs/`.
Its image hash identifies the expected file, not a device readback. Raw counts
can work while calibrated Motion remains invalid because tuning is unfinished.
Counts wrap as unsigned values; negative travel can display near 4294967295.
[Wheel trials](testing/subsystem2/logs/2026-10-15_wheel-measurements.json) own the
preliminary geometry, rejected trial and remaining accuracy checks.

Stopped MQTT mapping can be checked before PID setup: fresh matching Motion
health, acknowledged brake and at least 60 ms of valid quiet raw encoder
feedback are required. A mapping never starts movement.

Existing two-second raised-wheel commissioning modes also use IR-only
calibration, allowing later motor tuning without the missing sensors. They need
explicit electrical release and reviewed duty/speed settings under the normal
power policy. They remain disabled in the current configuration. Disable
commissioning before freezing the full mission image.

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

### Release gates: mission and submission readiness

The current branch is software-integrated, but it is not mission-ready or submission-ready. The dated [readiness record](testing/integration/logs/2026-10-15_available-readiness.json) and [test logs](testing/README.md) own evidence and status. Complete and record every applicable gate below. A build, fixture PASS, dashboard check, or hardware capability is not a physical mission PASS.

#### 1. Resolve inputs and measured configuration

- [ ] Resolve D01-D10 in the single [pending-decision register](docs/donotsubmit/PROJECT_SPECS.md#pending-decisions). Record the answer, measurement, evidence source, owner and acceptance threshold; do not replace evidence with a confirmation flag alone.
- [ ] D03/electrical: identify motor rated voltage/current (or obtain traceable part data), measure actual supply, and verify motor, driver, encoder supply/output levels and wiring compatibility. Confirm GP28 battery-sensing jumper state. Keep powered motor trials inhibited until this check passes. The carrier ratings in [TECHNICAL_INFO](docs/donotsubmit/TECHNICAL_INFO.md) are not the motor ratings.
- [ ] D03/D07, Motion: repeat marked left/right encoder counts and measure ground travel; confirm counts per wheel revolution, direction, effective distance per count, wheel diameter, track and car envelope. Tune and measure wheel PID, straight distance, turns, speed/saturation, stall handling, obstacle clearance and stopping/braking headroom.
- [ ] D04, Line/Barcode: verify installed IR module supply/output levels, polarity, height, contrast/noise and lighting range; calibrate thresholds and validate the issued barcode print, scale, direction, framing/quiet-zone/tolerance and mounting/traversal setup.
- [ ] D06, Terrain: install the IMU; verify supply, pull-ups/I2C and mounted axes; calibrate bias/filter/impact inputs and validate the height method against measured reference humps, including a one-wheel encounter.
- [ ] D05, Obstacle: the operator reports the professor confirmed the 3.3 V ultrasonic model. After the sensor is available, verify the installed marking and Pico-safe Echo level, measured range/repeat timing/missing-Echo handling. Calibrate servo supply/current, center/endpoints/settling, sonar offsets and scan geometry. The servo is present; IMU and ultrasonic hardware are currently unavailable for this team's testing, per operator update on 2026-10-15.
- [ ] D07/D08, Safety/Motion: measure brake command and physical stopping separately; verify stop/reset serialization, brake acknowledgement, stationary feedback, sensor/action deadlines, retries/leases, queue capacities and permitted recovery limits.
- [ ] D01/D02/D09/D10: confirm issued symbology and four mission-day letters procedure, barcode-to-junction spacing, crossing/finish/action-count convention, obstacle depth/bypass/reverse boundaries, broker/network settings, shared configuration/task-priority approval, agreed numeric acceptance limits, demonstration schedule, instructor access and submission evidence schedule.

#### 2. Close software and integrated verification

- [ ] Build the final frozen normal offline/wireless profiles and relevant inhibited TEST profiles; run strict changed-code checks, confirm fixtures are excluded from normal images, record configuration/source revision and UF2 hashes. Repeat if application code changes after the recorded software evidence.
- [ ] Execute all current firmware fixture IDs on the appropriate inhibited TEST image(s) built from the final source and retain actual console output; separately build and identify the final normal mission image. The recorded 45-check pass at `33ea475` used USB-only IR-only TEST with motors inhibited; it does not establish normal mission image behavior or physical acceptance.
- [ ] Complete INT-18/S1 transport tests: stopped/live mapping replacement and revision acknowledgement, moving/busy/invalid/backpressure rejection, failed QoS1 acknowledgement/reconnect/stale callback, overlapping requests/result-queue full, retained-message policy, reboot/reload on unchanged image, broker/Wi-Fi loss and recovery. The recorded reboot/reload and two-table checks are a passing subset; full transport/recovery remains partial.
- [ ] Complete INT-19 end-to-end controller integration: joint calibration, fresh local start, leased four-action execution, barcode capture/rearm and junction association, scan/cancel/center restoration, bypass and original-route recovery, finish and reset. Current unit/guard fixtures do not close this integration case.
- [ ] Complete T14/T15 fault and load checks: stale/missing/failed producer data, encoder faults/stall, expired actions, partial acquisition, ring/FIFO/ack queue saturation, network/reporting/console load, watchdog, release timing, lock contention, stack margin and measured stop latency. Confirm faults brake independently of telemetry and reset cannot replay commands.
- [ ] Recheck dashboard and telemetry under the final source/image and approved broker. Synthetic broker/browser checks are useful UI evidence; confirm current robot values/events and status requests on the demonstration network.

#### 3. Pass physical mission acceptance

- [ ] Execute T01-T02: combined black+IMU then white calibration; early/repeated/cancel/failure cases; GP21 stop in every mission phase; reset never starts motion; fresh local start requires qualified readiness.
- [ ] Execute T03-T05 and T11: line/curve/light/speed/junction tests; all A-Z at both scales and directions (104 combinations); invalid/unknown/duplicate/overflow cases; left/right/straight/U-turn at the correct junction; short/permanent line loss and bounded recovery.
- [ ] Execute T06-T07: labelled stationary/turn/climb/descent/impact motion and measured reference hump trials, including one-wheel and equal-height repeats; verify height error against agreed criteria and last/max/count/history across stop, reset and new run.
- [ ] Execute T08-T10: known obstacle range/material/angle and Echo fault trials; coarse/fine scans and cancellation; both bypass sides, blocked/unknown/tie cases, body clearance, confirmed depth/bounds, passed-obstacle strategy, wrong-crossing rejection and original-line reacquisition. Do not claim rear-edge observation unless the course method actually observes it or accepted course bounds explicitly support the strategy.
- [ ] Execute T12-T16 with the final normal wireless image: full robot telemetry/configuration/network recovery, required-data failure handling, concurrent load, and complete autonomous courses from calibration/mapping to confirmed finish. Meet agreed accuracy, clearance, reliability and time criteria; finish braked; retain failures and reruns. Rehearse different four-letter mappings and reboot/reload without changing the frozen image. Three clean rehearsals is a team target only, not an instructor rule.

#### 4. Complete submission evidence

- [ ] Prepare Motion tuning/accuracy and Terrain analysis reports from accepted measurements; include methods, references, results, error and limitations.
- [ ] Save final configuration, firmware/source revision, UF2 checksum, mapping request/revision, calibration, wiring/power, tester, test results and supporting console/MQTT/video/scope evidence. Every result must identify the image and distinguish firmware, software-integration and physical evidence.
- [ ] Confirm required artifact list, naming/format, repository or upload access, instructor access and due dates from course instructions. Exclude `docs/donotsubmit/` references from submission packaging; include demonstration video or link if required by the course.
- [ ] Obtain subsystem owner review and confirm the submission package contains the required reports, traceable acceptance records and final demonstration artifacts.

Declare **mission-ready** only when all applicable configuration/course decisions are resolved and physical M1-M10 acceptance passes on the frozen normal image. Declare **submission-ready** only when mission readiness is met and all required reports, evidence and artifacts are complete and accessible. If any required hardware, rule, threshold, or result is unavailable, mark the affected gate PENDING and do not claim readiness.

The diagnostic image remains motor inhibited. Do not use its IR-only calibration as evidence of full mission calibration or course readiness.
