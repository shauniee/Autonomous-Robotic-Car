# Mission readiness checklist

## Contents

- [DOUT navigation acceptance revision](#dout-navigation-acceptance-revision)

- [Phase 0: Electrical, before power (T01)](#phase-0-electrical-before-power-t01)
- [Phase 1: Boot, safety and MQTT (TEST firmware, wheels raised) (T02, T12)](#phase-1-boot-safety-and-mqtt-test-firmware-wheels-raised-t02-t12)
- [Phase 2: Each sensor alone (T03, T04, T06, T08)](#phase-2-each-sensor-alone-t03-t04-t06-t08)
- [Phase 3: Motion calibration (normal firmware, battery, on the floor) (T05)](#phase-3-motion-calibration-normal-firmware-battery-on-the-floor-t05)
- [Phase 4: Each feature on a test track (T03, T04, T07, T09, T10, T11, T13)](#phase-4-each-feature-on-a-test-track-t03-t04-t07-t09-t10-t11-t13)
- [Phase 5: Full mission rehearsal (T15, T16)](#phase-5-full-mission-rehearsal-t15-t16)
- [Phase 6: Final timing and looping acceptance (T14, T15, T16)](#phase-6-final-timing-and-looping-acceptance-t14-t15-t16)

Ordered physical checks that decide whether the robot is ready for a full run. Code and host tests cover only part of the project; these checks establish acceptance only for the recorded image, setup and limits. Work top to bottom: each phase assumes the earlier phases passed.

- Record every result as a row in [system/TEST_LOG.md](system/TEST_LOG.md) with the `MR-nn` ID, date, commit, tester, PASS/FAIL and the measured value, following [Run and record](README.md#run-and-record).
- **Record** names the value to write down and, where it replaces a placeholder, the constant in `app_program/common/robot_config.h` and the pending decision (Dnn) in [project-specs](../docs/project-specs.md#pending-decisions). After changing a constant, rebuild and repeat the checks that depend on it.
- Where no accepted target exists, the pass condition is "measured and agreed": Nicholas records the value and agrees the limit in the pending-decisions register. Never pass a check on a build or a host test alone.
- The `T` column links each check to the planned integrated trials in [testing](README.md#planned-integrated-trials).

Current procedure revision: 2026-11-03, aligned to the [pending work](../pending.md).
Existing results retain their historical procedure/source meaning; this page does
not convert old observations into target passes. The linked pending work tracks
physical acceptance; issue #117 remains closed. Agree pending numeric limits before corresponding PASS decisions.

## Phase 0: Electrical, before power (T01)

Wrong wiring can damage the Pico or a sensor, so nothing is powered until this phase passes.

| ID | Check | Pass when | Record |
| --- | --- | --- | --- |
| MR-01 | Every wire against the pin table in [team](../docs/team.md#2-pins), by continuity | All match; no unclaimed pin is connected | Closed by Nicholas, 2026-11-05: checked and unchanged |
| MR-02 | Grove 6 GP26 contact disconnected; GP28 battery-sense jumper open | No continuity on either | Closed by Nicholas, 2026-11-05: checked and unchanged |
| MR-03 | Sonar Echo HIGH level, measured before connecting to GP3 | At most 3.3 V | Closed by Nicholas, 2026-11-05: always 3.3 V, not measured |
| MR-04 | Power bank polarity at VIN+/VIN-; one power source at a time | Correct polarity; USB disconnected during battery runs | - |
| MR-05 | 3V3 and VSYS while both motors and the servo run | Measured and agreed; no brownout reset | Closed by Nicholas, 2026-11-05: no brownout, volts not measured |

## Phase 1: Boot, safety and MQTT (TEST firmware, wheels raised) (T02, T12)

Proves the robot cannot move unexpectedly and that the laptop link works before anything drives.

| ID | Check | Pass when | Record |
| --- | --- | --- | --- |
| MR-06 | Flash the TEST image on USB | Console prints the `[TEST]` lines; motors stay braked | Console log |
| MR-07 | Press GP21 while stopped or while in an inhibited TEST start; after a serviced brake and cleared causes, release GP21; then press GP20. For a fault-induced stop, make a **new GP21 press/release** after clearing the fault; test held buttons | GP21 release (when eligible) returns STOPPED to READY without driving; a fresh GP20 press alone starts only when all start gates pass; a persistent fault cannot reset; TEST never drives | Record button edges, stop latch, brake acknowledgement and state transitions |
| MR-08 | `mqtt_client.py monitor` with the wireless image | A TELEMETRY line about every 500 ms with `mapped=NO` | Saved monitor output |
| MR-09 | Press GP20 to start without a mapping | Console `start refused: no barcode mapping`; state stays READY | - |
| MR-10 | `mqtt_client.py map A B C D` while stopped; repeat while following; reboot | `ACK ... OK` and `mapped=yes`; `BUSY` while moving; `mapped=NO` after reboot | - |
| MR-11 | Power off, disconnect IMU, boot, install mapping and attempt a fresh GP20 start on a valid DOUT line | Bounded IMU startup attempt reports failure before readiness; no black/white IR software calibration exists. The IMU fault is reported but start is allowed once required DOUT IR/Motion/mapping/WiFi/MQTT and safety gates qualify. Reconnect with power removed | Boot timeout, fault byte, READY/FOLLOW transition |

## Phase 2: Each sensor alone (T03, T04, T06, T08)

Each value measured here replaces a placeholder that later behaviour depends on.

| ID | Check | Pass when | Record |
| --- | --- | --- | --- |
| MR-12 | Read **digital DOUT** from both mounted line sensors (GP26/GP27) over black, white, a centred line and each side at course lighting; test invalid/stale/disconnected reads. **Do not perform ADC black/white capture** or use GP20 for IR calibration | Each sensor switches reproducibly between 0 and 1, the observed black level agrees with `IR_BLACK_LEVEL`, and fresh valid readings produce expected VISIBLE/BROAD_BLACK/white-white guidance. Missing/stale acquisition is rejected; GP20 remains start-only | Left/right DOUT levels, mounting/lighting and sample ages (D04) |
| MR-13 | Barcode IR (GP16) level over black and over white | Level differs consistently | Which level means black: `BARCODE_BLACK_HIGH` (D04) |
| MR-14 | Turn each wheel one revolution forward by hand, three times | Counts increase on both wheels, repeatably; no encoder fault | Record counts/revolution in project specs; effective rolling scale: `ENC_COUNTS_PER_MM`; signs: `*_ENCODER_SIGN` (D03) |
| MR-15 | Force an encoder fault (wiggle a connector), then spin the wheel by hand | `Motion: encoder` appears, then clears after spinning | - |
| MR-16 | Boot with the robot still; tilt the front up; tap the chassis sharply | No calibration fault; pitch rises when the nose is up; `Terrain: impact` and a latched stop on the tap | Pitch sign; tap level that triggers (D06) |
| MR-17 | Sonar against a flat board at 100, 250, 500 and 1000 mm; then aim at open space | Reported range measured and agreed against a tape measure; open space shows `no echo` and the robot is **not** stopped | Error at each distance (D05) |
| MR-18 | Servo to centre and to both sweep limits | Centre faces forward; limits do not stall the servo | `SERVO_CENTRE_US`, `SERVO_US_PER_DEG`, settle time (D05) |

## Phase 3: Motion calibration (normal firmware, battery, on the floor) (T05)

Turns, distances and barcode widths all come from encoder travel, so these values must be right before any feature test.

| ID | Check | Pass when | Record |
| --- | --- | --- | --- |
| MR-19 | Short forward command on each motor | Both wheels drive forward | `LEFT_FORWARD_A`, `RIGHT_FORWARD_A` (D03) |
| MR-20 | Measure wheel diameter and track width with calipers | - | Record diameter as hardware evidence; `TRACK_WIDTH_MM` (D03); odometry uses direct rolling calibration |
| MR-21 | Drive 1 m straight five times at follow speed | Distance and lateral error measured and agreed (Tribunal proposal: lateral error at most 10 mm) | Errors; PID gains `MOTION_KP/KI/KD` (D07) |
| MR-22 | 90 degrees left, 90 degrees right, 180 degrees; five each | Final heading error measured and agreed (Tribunal proposal: at most 5 degrees) | Errors per turn (D07) |
| MR-23 | Repeat MR-22 with one wheel on a hump | Error measured and compared with MR-22 (M3 requires accuracy to hold) | Errors with one wheel raised |
| MR-24 | Press GP21 at follow speed | Robot stops | No distance required (Nicholas, 2026-11-05) |
| MR-25 | Use the full-course image, which has the guard on, with wheels raised and controlled load | `Motion: motor stall` and a latched stop; cause-clear GP21 press/release returns READY without restarting | Time to detect (`MOTION_STALL_MS`, D07) |

## Phase 4: Each feature on a test track (T03, T04, T07, T09, T10, T11, T13)

Each feature is tested alone before the full course.

| ID | Check | Pass when | Record |
| --- | --- | --- | --- |
| MR-26 | Follow a loop with straights and curves, three laps (M1) | Stays on the line every lap | Self-tested by Nicholas (2026-11-05); no recorded rate required |
| MR-27 | Printed course barcodes in both directions at operating speed; verify incomplete capture rejects rather than invents a character (M2) | Same correct character/action in both directions; one pending action per encounter; no two-second blind delay or guessed character | Reads, misses and misreads per letter (D01) |
| MR-28 | Barcode before a cross junction for each of left, right, straight and U-turn (M3), with measured `NAV_SENSOR_FORWARD_MM`, `NAV_CROSSBAR_MM`, encoder scale and signed geometry | One queued action executes at the qualified junction at variable barcode distances. For turns: measured approach to axle-centre, stationary pause, line-confirmed pivot and outgoing tracking; U-turn pause/limits validated. Do not accept a turn from an unqualified broad-black curve or an uncalibrated geometry release | Measured entry/skew, advance distance, pivot stop/overshoot and outgoing-line success (D02/D03). All four actions work (Nicholas, 2026-11-05), observed only |
| MR-29 | Reference humps of known heights up to 30 mm ([hump requirement](../docs/project-specs.md#course-rules)): the lower practice ramp, one slightly below and one at or near 30 mm; three passes each (M4, M5), including a single-wheel case. First confirm on the robot that a 30 mm-or-lower single-wheel ramp crosses the climb-entry threshold | 100 mm/s held through the encounter; count/height/max/events correct; reference error and valid-completion behavior measured and agreed | Reported vs actual per hump (D06) |
| MR-30 | Reference obstacles across 100-200 mm width, offset left and right of the line; vary position/background/material (M6-M9) | Stops, scans, bypasses on the side with the shorter distance to the box edge, reports width and depth, rejoins the original line | Width/depth error, repeatability/uncertainty, chosen gap, clearance, bypass time and reacquisition success (D02, D05, D10) |
| MR-31 | Boxes blocking both sides | Robot stops instead of bypassing | - |
| MR-32 | Induce healthy-sensor line loss while following; separately freeze/fail acquisition | Brake and confirm stationarity, then the current bounded wider left/right pivot sweep with braking before reversals; either sensor initiates alignment, stable intended-line tracking resumes; exhausted search stops; stale acquisition does not enter blind search | Sweep sequence, travel, time and false-line acceptance (D07) |
| MR-33 | Stop the broker during a run, then restart it | Robot keeps driving; `Network: broker lost` shows after reconnect | - |
| MR-34 | Retired: former finish-marker check (robot stops at finish; finish convention was undecided) | Not run/reused for the endless target; use newly allocated MR-40 | Preserve any historical MR-34 evidence |

## Phase 5: Full mission rehearsal (T15, T16)

The assessment rewards repeatable runs, so the robot is mission-ready only when this phase passes three times in a row.

| ID | Check | Pass when | Record |
| --- | --- | --- | --- |
| MR-35 | Mission-day routine: fresh boot, **automatic bounded stationary IMU startup attempt**, verify digital DOUT IR, join WiFi/MQTT, map unfamiliar letters (e.g. A straight, Z U-turn, W left, F right), then fresh GP20 start | Mapping acknowledged after installation without recompiling; absent WiFi/MQTT or invalid/stale DOUT IR/Motion blocks start; IMU failure is reported under its designated fault policy | Time from boot to start and exact readiness/stop causes |
| MR-36 | Full course with line, barcodes, junctions, humps and an obstacle | Observed loop session completes its planned encounters, every junction action correct without reversal, hump/obstacle values reported; GP21 ends session, no automatic COMPLETE | Run time (about 5 minutes expected); features working |
| MR-37 | Repeat MR-36 twice more without changing code or constants | Three consecutive successful runs | Run times and any differences |
| MR-38 | Rebuild the route from the saved `monitor` output | Event times and encoder counts give the order of barcodes, turns, humps and obstacles | Saved output and reconstruction |

## Phase 6: Final timing and looping acceptance (T14, T15, T16)

These future cases claim no result. MR-39 was already referenced in #117;
MR-40 is a newly allocated endless-operation check, not reuse of retired MR-34.

| ID | Check | Pass when | Record |
| --- | --- | --- | --- |
| MR-39 | Final-image concurrent load: line acquisition, Mission/Motion, IMU timeout, barcode/Echo IRQ, servo, telemetry and reconnect; separate TEST and normal loads | Agreed D13 periods/budgets/margins met; no required stale-data replay; local stop independent of queue load | Exact image/profile, line-to-output latency, WCET/release jitter, missed deadlines, IRQ edge rate/time, stack/RAM and ring high-water marks; host/TEST is not full motor-load acceptance |
| MR-40 | Revisit the same loop junction with/without pending actions; run repeated feature cycles, then GP21; interrupt WiFi/MQTT mid-run | No automatic finish or undo; healthy local run survives network loss; new start requires WiFi/MQTT; GP21 stops, eligible release returns READY, fresh GP20 starts | Session encounters, stop reasons, action/event order, reference video and unchanged image/constants |

The complete remaining-work checklist is [pending work](../pending.md).
This file owns physical procedures, not a second completion tracker. For final
Motion/Mission/I/O timing, agree D13 limits and a bounded capture method; old
`s3_input`/`s3_mission` console windows are not current implementation evidence.

## DOUT navigation acceptance revision

This supersedes earlier MR-12 analogue calibration and MR-28 fixed/instant turn
procedures; do not overwrite historical results. Define new test IDs before running.
Measurements and release prerequisites are listed in
[specs](../docs/project-specs.md#dout-navigation-commissioning).

1. Record DOUT levels/dead zone while sliding across the actual line at mounted
   height; record each wheel rolling scale/sign, equal-target drift and speed response.
2. Record straight sway and curve/S-bend overshoot with progressive steering;
   keep settings fixed while tuning the wheel loops first.
3. Record actual digital barcode edge widths, correct/rejected/wrong letters and
   both print orientations during tracking. Confirm a complete read precedes junction.
4. Record false broad-black curve candidates and straight/slanted/curve-to-junction
   entries for every direction. Unknown/curved turn geometry must stop without
   pivot fallback. With geometry unreleased, queued straight and unqueued crossings
   must keep moving and resume tracking on fresh non-broad input, including a
   centred white/white exit without an edge excursion.
5. With measured geometry released, measure chassis-centre and heading errors after
   mirrored turns, body clearance, speed transitions, exit evidence and rearming.
6. Measure U-turn approach position, stationary pauses, pivot profile and return-line
   validation; test intermediate branches, stop, obstacles and stale data in each phase.
7. Include junction-to-curve and one-wheel hump trials. Choose numeric acceptance
   limits from actual clearance and barcode capture margins before release.

No physical trial can be completed without the robot; current software evidence
is [INT-95](integration/TEST_LOG.md#results).
