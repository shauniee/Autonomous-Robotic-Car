# Autonomous robotic car — project plan

## Goal and working rules

Use the [design review](<../../CSC01 Design Review.docx>) as the primary design and its T01–T16 trials as the verification plan. Safety 10 ms, control/sensing 20 ms and commanded brake within 50 ms are proposed targets, not measured passes. [Current implementation](IMPLEMENTATION.md) separates completed builds from pending hardware/full-mission evidence.

Build and demonstrate the integrated autonomous mission described in [project-info.md](project-info.md), using the component limits in [tech-info.md](tech-info.md) and the consolidated wiring and system contracts in [architecture.md](ARCHITECTURE.md). The briefing calls for a **Week 10 demonstration**; the team also reported roughly a one-month working horizon. There are no supplied calendar dates, marking weights, or numerical pass thresholds, so the sequence and checkpoints below are team planning proposals.

Keep one shared firmware application and merge small, measured subsystem increments early. Each owner supplies a usable API, calibration data, and repeatable evidence, then helps integrate the full car. Record results in [`code/docs/test-log.csv`](test-log.csv): firmware revision, setup, expected result, measured result and units, pass/fail, and evidence link. Agree on a measurable pass condition *before* each trial. Mark untested functions as untested rather than treating desktop checks as hardware proof.

## Stage 0 — settle the physical design and team contract

**Exit checkpoint:** a reviewed wiring sheet, safe power/logic measurements, agreed interface conventions, and an integration owner.

- [x] Design review assigns Nicholas as integration/Mission/Safety owner and records each buddy's responsibility: Shaun communications, Mahesha motion, Nicholas line/barcode, Darrius terrain, Jia Jing obstacles.
- [ ] Walk through the connection table in `architecture.md` against the actual carrier silkscreen and installed module labels; update the design if the built car differs. Identify exact IR boards, encoder wiring/output levels, power-bank rating, motor model/current, and SG90 variant. Keep the shared GP26 contact on Grove 6 disconnected and confirm GP28 battery-sense jumper is open.
- [ ] Confirm the installed sonar is the reported **HC-SR04P**, which supports 3.3 V on [a manufacturer-listed variant](https://www.okystar.com/product-item/hc-sr04-ultrasonic-sensor-module-oky3261-2/38/). Measure Grove 2 3V3 and Echo HIGH before attaching GP2; direct Echo is acceptable only if it remains within the Pico's 3.3 V limit. Check that GP3's 3.3 V Trig works. The supplied 5 V HC-SR04 sheet describes an older, different variant.
- [ ] Measure VIN polarity/voltage, 3V3 and servo rails under load, IR AOUT highs, encoder highs, and I²C pull-up highs; check the aggregate sensor current and motor current against carrier limits. Perform the first motor tests with wheels raised and a known physical stop.
- [ ] Confirm track direction, line/barcode sensor positions, actual barcode symbols and command mapping, A/Z sample relevance, course boundary, obstacle rules, hump reference heights, finish condition, and whether U-turn is assessed. Keep the system-level U-turn requirement in scope until clarified.
- [ ] Agree on units, sample validity/age, one-shot events, timeout/stop behavior, API ownership, and measurable acceptance targets. Record decisions in the relevant source/design docs rather than scattering them across temporary notes.

## Stage 1 — establish a repeatable build and bench baseline

**Exit checkpoint:** the desktop controller check passes, Pico WH firmware builds/flashes, and USB status plus safe stop are observed on the actual board.

- [ ] Review [`code/README.md`](README.md), `code/include/robot/`, and the controller/scaffold. Run the PowerShell desktop check on a machine with GCC (`powershell -ExecutionPolicy Bypass -File scripts/check.ps1` from `code/`); capture its controller test result and synthetic output. This is logic evidence only.
- [ ] Set up Pico SDK, ARM toolchain, CMake, and Ninja; build `code/` with `PICO_BOARD=pico_w` for the installed Pico WH. Flash the generated `robot_car.uf2` by BOOTSEL and verify actual USB serial status. The current `s`/`x` inputs are bench controls; with invalid sensor stubs, start should fault and stop should latch.
- [ ] Add a physical/driver stop path before any motor motion. Keep output disabled on boot, fault, stale command, and controller stop. Add a bounded motor-command timeout and assess watchdog behavior when real outputs are introduced.
- [ ] Log one timestamped raw reading per connected sensor and an encoder count from each wheel, without asserting calibrated mission behavior. A driver returning zeroed data must remain invalid until it measures real input.

## Stage 2 — parallel subsystem increments and calibration

Owners can work in parallel after Stage 0's wiring and interface decisions. Integrate each increment as soon as it returns trustworthy readings. Each deliverable below needs a short repeatable test, measured values, and a clear failure case.

| Owner | Build tasks | Evidence to hand over |
| --- | --- | --- |
| Mahesha — motion | Verify M1=left/M2=right and actual forward polarity; count both A/B encoder channels; measure wheel diameter, wheel separation, decoded counts/revolution; implement stop, speed/distance feedback, per-wheel PID, straight correction, bounded distance/turn actions. | Commanded versus measured speed; distance/turn error over repeated runs; stop and stall/timeout behavior; PID tuning record. |
| Nicholas — line/barcode | Calibrate left/right analogue IR on GP27/GP26 at mounted height; map polarity and normalized line error; add loss/junction detection and low-speed steering; sample GP16 barcode transitions; decode ordered bar **and space** widths using encoder distance or speed compensation; reject partial reads and emit once. | Black/white raw ranges, line-run and loss-stop trials, known A/Z scans at different speeds/scales, final command mapping and four required turn events. |
| Darrius — terrain | Verify GY-511 I²C addresses and mounted axes; configure fresh acceleration samples; calibrate offsets; detect climb/descent, hump and impact; design a physical-height estimator with reference humps; track the maximum **valid** height. | Timestamped raw traces, false-positive trials, reference-height versus estimate/error table, highest-peak report. Do not claim raw gyro yaw rate: LSM303DLHC has no gyro. |
| Jia Jing — obstacle | Bring up HC-SR04P forward range with a conservative >60 ms starting interval, then measure valid trigger spacing and bounded echo timeout; calibrate servo angles/settling; make coarse/fine scans; estimate closest point, width and side clearance with car width/margin; propose bounded bypass/reverse; search and reacquire the original line. | Forward-distance comparison, scan profile trials, no-echo/blocked-side behavior, repeated bypass and line-recovery runs. |
| Shaun — communications | Define broker/network, authentication and MQTT schema; publish measured state/sensor/motion values with validity, heartbeat and reconnect state; validate any subscribed messages. Keep remote driving and remote start/stop out of the mission design. | Actual broker messages, payload examples, disconnect/reconnect trial, bounded network-work timing, telemetry demonstration. |

Develop Shaun's MQTT work alongside the sensors and motion; use USB measurements early so network setup does not prevent hardware calibration. Maintain one owner of motor outputs and one owner of mission arbitration throughout.

## Stage 3 — integrate behavior in increasing scope

**Exit checkpoint:** every target state is entered and exited using real observations, with bounded time and a safe fallback.

1. [ ] Wire valid line, forward-range, terrain, and motion feedback into the controller. Verify stop on physical command, collision/fault, stale or invalid required readings, and motor timeout. Revisit scaffold freshness limits using measured sensor periods; a full servo scan cannot be treated as an instant sample.
2. [ ] Tune line tracking at low speed, then normal speed on a straight and curve. Verify line-loss stop/recovery and lighting variation before adding navigation decisions.
3. [ ] Add barcode-triggered left/right/straight/U-turn actions with encoder completion and turn timeout; suppress repeated reads of one symbol, then reacquire the line after each maneuver. The current scaffold lacks `NAV_U_TURN` and stops for any navigation event.
4. [ ] Run hump detection and maximum-height estimation alongside navigation. Keep `height_valid=false` until a reference-based method is calibrated; use encoder turns rather than claiming a nonexistent gyro reading.
5. [ ] Add obstacle slow/stop, scan, clearance choice, bypass, and line reacquisition. Keep forward clearance current while side scans run. Test blocked routes and missing echoes; stop safely when no justified path exists.
6. [ ] Add WiFi/MQTT telemetry to full-route runs. Decide and test behavior when the broker or WiFi is unavailable. Network work must not delay motor updates or override local safety.

## Stage 4 — full mission, evidence, and demonstration

**Exit checkpoint:** the car completes the agreed course repeatedly, reports the required observations, and the team can show what was measured and how faults were handled.

- [ ] Freeze the final course interpretation and numerical acceptance targets: line completion, command accuracy, turn/distance error, hump-height error, obstacle clearance/recovery, and telemetry availability. The assignment provides no numeric thresholds, so label these as team targets.
- [ ] Run repeated end-to-end trials across the actual straight, curve, T-junction, barcode, hump, and obstacle placements. Log success/failure per run and change only one calibration variable at a time where possible.
- [ ] Exercise recovery cases: line loss, unreadable barcode, no sonar echo, blocked sides, stale sensor, encoder fault/stall, WiFi loss/reconnect, and low or unstable power. Confirm every unsafe case stops or enters a bounded recovery.
- [ ] Rehearse the Week 10 demonstration with a freshly powered car, known starting condition, saved logs/telemetry, and each buddy able to explain their interface and evidence. Document the integrated result and remaining limits honestly.

## Decisions and dependencies to close

| Decision | Why it blocks progress |
| --- | --- |
| Actual module labels, current draw, sensor output voltage, and shared Grove contacts | Determines whether proposed adapters and power budget are valid. |
| Barcode symbol set, orientation and symbol→command table | Required before decoder and navigation tests have a real expected result. |
| Wheel geometry/counting convention and turn/stop tolerances | Required to calibrate motion, barcode widths, and bypass. |
| Hump reference method and acceptable height error | Required to report a physical peak height rather than just a detection. |
| Obstacle course boundary, car width and safety margin | Required to call a scan path passable and validate bypass. |
| Finish rule and integrated MQTT deployment/schema | GP20/GP21 local controls and continued healthy local autonomy on network loss are set by the review. Bench broker/schema are configured; final mission deployment and course finish remain open. |

If a decision remains open, continue the independent sensor and bench tasks, and keep the dependent feature marked pending. Do not fill a missing specification with an untested constant from `code/include/robot/config.h`.
