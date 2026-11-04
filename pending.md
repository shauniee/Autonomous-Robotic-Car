# Pending work

## Contents


- [How this list is ordered](#how-this-list-is-ordered)
- [Where everything lives](#where-everything-lives)
- [Planned changes](#planned-changes)
- [Where things stand](#where-things-stand)
- [What needs the robot](#what-needs-the-robot)
- [Required coverage](#required-coverage)
- [Decisions for Nicholas](#decisions-for-nicholas)
- [1. Safety and electrical](#1-safety-and-electrical)
- [2. Motion calibration](#2-motion-calibration)
- [3. Line tracking (M1)](#3-line-tracking-m1)
- [4. Barcode and junctions (M2, M3)](#4-barcode-and-junctions-m2-m3)
- [5. Obstacle and bypass (M6 to M9)](#5-obstacle-and-bypass-m6-to-m9)
- [6. Hump (M4, M5)](#6-hump-m4-m5)
- [7. Telemetry and mapping (M10)](#7-telemetry-and-mapping-m10)
- [8. Faults seen and not explained](#8-faults-seen-and-not-explained)
- [9. Full mission and repeatability](#9-full-mission-and-repeatability)
- [10. Week 13 evidence (minimum-bit, timing, resources)](#10-week-13-evidence-minimum-bit-timing-resources)
  - [Bit reduction candidates](#bit-reduction-candidates)
- [11. Reports, demonstrations and traceability](#11-reports-demonstrations-and-traceability)
- [12. Stretch goal](#12-stretch-goal)
- [13. Housekeeping](#13-housekeeping)
- [Complete](#complete)
- [Superseded](#superseded)

## How this list is ordered

Most important first, in sections and within each section. The order is by importance, not the quickest next step:

1. What can damage the robot or make it unsafe.
2. What every other test depends on.
3. Course features, because the demonstration scores features working. Obstacle comes before hump because the professor's clarification calls bypass and recovery central, and bypass has never run.
4. Whole-mission repeatability, then required Week 13 evidence.
5. Reports, stretch goals and housekeeping.

IDs: `M` requirement, `MR` readiness check, `D` pending decision, `T` integrated trial, `A` Tribunal assumption, `R` Tribunal risk, `Gap` Tribunal Version C. `docs/tribunal1.md` is frozen; differences from it are listed under [Superseded](#superseded).

## Where everything lives

This file is the one list of changes and planned changes. `main` stays as it is, because it works. Everything unmerged is on `chore/centralise-pending`, which is built and host-tested but **has never run on the car**. Do not flash it for a demonstration in place of `main`.

| Source | Purpose | State on `chore/centralise-pending` |
| --- | --- | --- |
| `origin/fix/imu-bugs` (`abb1f05`) | `TERRAIN_DIAGNOSTICS=1` raw IMU trace and `scripts/capture_imu.py` | Merged. Use this one: the local `fix/imu-bugs` is the older copy |
| `origin/fix/pending-software-readiness-20261104` (PR 151) | Testable watchdog feed rule, GitHub host tests, build matrix script, report drafts | Merged. Barcode rule combined with the one below; report tables moved into the Motion and Terrain READMEs, because `docs/` holds three guides only |
| `chore/resolve-pending`, same commit as `origin/fix/pending-nic1` | Bit cuts, stall guard on for the full course, capture analysis, Nicholas's decisions | Merged |
| `fix/pending-nic2` | Stricter lap scorer and capture triage, qualifying lap and minimum-bit audit | Merged. Test rows renumbered |
| `fix/crossing-exit-steer` (`526e5b5`) | Straight crossings: ramped 36 mm advance, 6 mm candidate, no false bend on regaining the line | Merged with 16-bit encoder origins. Test rows and log folders renumbered |
| `chore/audit-dead-code` (`a1ddfab`) | Removes `TerrainSnapshot`, `LPF_ALPHA`, `MISSION_CONFIRM_SAMPLES`, the empty obstacle test and the old plan checker | Redone by hand; comment-only edits from that commit were not carried over |

Reused test IDs, renumbered because an ID is never reused:

| Branch | Old | New |
| --- | --- | --- |
| `fix/pending-nic2` | INT-162 to INT-169 | INT-179 to INT-186 |
| `fix/crossing-exit-steer` | INT-162 to INT-167 | INT-187 to INT-192 |

Commit hashes in those rows still name the original commits.

Already merged or closed, so nothing is left to bring in: `test/obstacle-width-bench` (#150), `fix/line-steering-strength` (#148), `chore/short-uf2-names` (#146), `chore/no-co-author` (#145), `chore/common-cleanup` (#147, closed, reached `main` through #148).

## Planned changes

Nothing else changes firmware without the robot, because every remaining value needs a measurement. In order:

1. Nicholas answers the [decisions](#decisions-for-nicholas) that need no robot.
2. With the robot, flash a `TEST=1` image from `chore/centralise-pending` and run [section 1](#1-safety-and-electrical) and [section 2](#2-motion-calibration).
3. Run the crossing trial and the bit-cut checks in [Bit reduction candidates](#bit-reduction-candidates) on the car. If one fails, compare against `main`, which is the known-good image.
4. Run sections 3 to 9, changing one value per trial and recording each as a new INT row.
5. Measure the final image for [section 10](#10-week-13-evidence-minimum-bit-timing-resources), then finish [section 11](#11-reports-demonstrations-and-traceability).
6. Open one PR from `chore/centralise-pending` once the car checks pass. PR 151 and the source branches can then close; each owner deletes their own.

## Where things stand

| Area | State | Evidence |
| --- | --- | --- |
| Firmware build | `main` is the working image and is unchanged. On `chore/centralise-pending`, all seven images and the IMU diagnostic image build and the host suite passes. Not a firmware or physical pass. | INT-193 |
| Safety and electrical | No recorded result. Wiring is user-confirmed only. | MR-01 to MR-05 |
| Watchdog | Code merged, off by default, never run on a Pico. No reset policy agreed. | `99fe0d8`, D15 |
| Line tracking | Straight-crossing fix not yet run on the car in its final form (INT-191). Good laps seen, but the last run went off course and was not smooth. No measured success rate. | User observation, INT-136, INT-139b, INT-158; capture analysis found no cause (INT-163) |
| Barcode | User reports fully functional. All letters and a flipped print not tested. | User observation, INT-156 |
| Junctions | Some turns passed; junction failures also logged. | INT-137, INT-139b, INT-153 |
| Obstacle | Scans ran on real boxes with a measured width error. Bench check pending, and bypass has never run. | S5-U10, S5-U11 |
| Hump | Latest fix built, never run on the car. Last run counted 5 for 1 crossing. | INT-158, host and build only |
| Telemetry | Binary link in use on the bench. Physical subscription check pending. | S1-M01 |
| Full mission | Never run as an acceptance check. | MR-35 to MR-40 |
| Timing and resources | Not measured on the final build. | SYS-11, SYS-12 (October builds) |

## What needs the robot

Software investigation, fixes, host tests, Arm builds, static RAM/layout analysis,
protocol fixtures, report structure and procedure preparation can happen now.
They cannot close these physical items:

| Blocked acceptance | Why the robot or actual environment is needed | Checklist coverage |
| --- | --- | --- |
| Electrical and stopping safety | Continuity, Echo voltage, power rails under load, braking distance/output waveforms, impact, held-button/reset and watchdog behavior need hardware measurements. The loose-battery restart cause is already explained. | Section 1; MR-01–07, MR-16, MR-24/25, SYS-06 |
| Motion and encoder calibration | Actual wheel travel/sign, edge errors/rates, geometry, floor PID, slip and turn accuracy cannot be inferred from raised-wheel or host tests. | Section 2; MR-14/15, MR-19–23 |
| Robust line/barcode/junction behavior | Mounted sensor levels, lighting, issued prints, physical overflow rate, three curves, action hand-over, TRACE effect and untouched 100 mm/s laps require the course. | Sections 3–4, 8; MR-12/13, MR-26–28/32 |
| Hump accuracy and reliability | Current fix needs actual crossings; height scale/error, reverse/curve/pivot effects, minimum detectable hump, stuck detection and disconnected-IMU behavior require measured references. | Section 6; MR-11/16/23/29 |
| Obstacle and full bypass | Servo endpoints, sonar error, swept envelope, width/depth, blocked sides and original-line rejoin have no accepted integrated physical evidence. | Section 5; MR-17/18/30/31 |
| End-to-end instructor telemetry | Laptop fixtures can prove codec behavior, not Pico radio timing, reconnect, real mapping/fault injection or access to the instructor network. Credentials alone do not prove connectivity. | Section 7; MR-08–10/33/38 |
| Loaded resources and repeatability | WCET/jitter/ISR/lock cost, live stack/peak RAM, actual queue high-water marks, three full runs, 20-lap samples and physical videos require the running final image. | Sections 9–11; MR-35–40 |

## Required coverage

`docs/tribunal1.md` has no table labelled FR/NFR. Until Nicholas identifies another
source, this plan conservatively retains all M1–M10 from the owning requirements
guide plus the Tribunal's functional, safety and resource obligations. These
labels are a crosswalk, not invented instructor requirement IDs.

| Requirement / obligation | Plan coverage and closure |
| --- | --- |
| M1 line tracking and lighting tolerance | sections 2–3; qualifying lap plus measured repeatability |
| M2 A–Z in either direction, stored until next junction | section 4; both traversal directions, one action/event per read, physical print coverage |
| M3 all four encoder actions, including one wheel on hump | sections 2, 4 and 6; ground distance/angle measurements and navigation trials |
| M4/M5 detection, each height/count and retained maximum | section 6; reference measurements, exact crossing count, no pivot false positives |
| M6/M7 obstacle detection, profile, clearance and depth | section 5; measured boards/boxes and side/rear-edge evidence |
| M8/M9 safe bypass and original-line continuation | section 5; both sides, restricted/blocked cases, envelope and correct rejoin. Rejoining during BYPASS/NAVIGATE is required; a standalone REACQUIRE during the qualifying lap fails it. |
| M10 two-way MQTT, runtime mapping, readable telemetry and route evidence | section 7; instructor broker, malformed requests, BUSY/reboot, fault visibility and reconnect |
| Autonomous, undisclosed route, endless operation, micro T-Kernel | sections 9/11; unfamiliar runtime letters, repeated encounters, no remote driving or hard-coded route |
| Single motor writer, stop precedence, freshness, reset and fault independence | sections 1/8/10; separate intentional fault/stop tests; networking has no movement authority |
| Race-free minimum-bit data, precision, invalid/overflow behavior | section 10 and Tribunal Gaps 1–7; field census, accessor/boundary checks, final sizes and on-target timing |
| Electrical, timing, stack, performance and repeatability | sections 1/9/10 and Tribunal risks R1–R5; measured values, not host inference |
| Reports, demonstrations and traceability | section 11 and Tribunal Gap 8; unique result IDs, exact source/image, saved measurements/video links |

## Decisions for Nicholas

All questions and decisions go to Nicholas. Proposed defaults are recorded as proposed, not as sign-off.

- [ ] Print framing, quiet zones and tolerances (D01).
- [ ] Error limits for lateral, turn, height, width and recovery (D10), and the watchdog reset policy (D15).
- [ ] Data table: fault bytes accepted at 48 bits. Test the [bit reductions](#bit-reduction-candidates), then sign off each group.
- [ ] Keep diagnostic frames 9, 10 and 11.
- [ ] Network (D09): two records disagree, your own WiFi and laptop broker or the instructor's. Only `config/wifi_secrets.h` differs.
- [ ] Obstacle course speed (D07): bypass moves slower than 100 mm/s, so a whole obstacle lap cannot claim 100 mm/s unless the qualifying lap allows it.
- [ ] Measured-speed variation allowed on a qualifying lap (D07, D10).
- [ ] U-turn pause: 100 ms trial value, about 200 ms requested (D02).
- [ ] Inverted print (white bars on black): proposed unsupported.
- [ ] `TRACE=0` as the final default, after the A/B laps.

## 1. Safety and electrical

- [ ] Sonar Echo high level measured at 3.3 V or below (MR-03, R5, D05). A higher level can damage the GPIO.
- [ ] Every wire checked by continuity, with a photo (MR-01, A7).
- [ ] GP26 Grove contact disconnected and GP28 jumper open (MR-02, A7).
- [ ] Power bank polarity, and one power source at a time (MR-04).
- [ ] 3V3 and VSYS measured with both motors and the servo running; no brownout (MR-05, A6).
- [ ] GP21 at follow speed: robot stops, and the stopping distance is recorded (MR-24, D08).
- [ ] Measure output-brake latency against the 50 ms target, separately from physical stopping (A11, T02).
- [ ] Reset cannot start the robot: held buttons, persistent fault, expired and old commands, inhibited boot (MR-07, A12, Gap 4).
- [ ] TEST image on the current build: `[TEST]` lines print and motors stay braked (MR-06).
- [ ] Stall guard: on for the full-course image (INT-176), off on `main`, which the demonstration uses. Confirm on the car that it causes no false stop, and that a held wheel stops the run (MR-25, D07).
- [ ] Watchdog: agree the reset policy (D15), then test a `WATCHDOG=1 TEST=1` image with wheels raised: reset on a stopped task, motor outputs through reset, no restart without GP20, and no false reset under WiFi load (SYS-06). The code is merged but off by default; do not enable it on a driving car until this passes.
- [ ] Impact: tap test gives `Terrain: impact` and a latched stop (MR-16, D06).

## 2. Motion calibration

Turns, distances and barcode widths all come from encoder travel.

- [ ] Each motor drives forward on a short command (MR-19, D03).
- [ ] Encoder counts per revolution, both wheels, repeated across speeds and reversals (MR-14, A2, D03). Only manual trials so far.
- [ ] Forced encoder fault appears and clears (MR-15).
- [ ] Measure wheel diameter and effective track width; only wheel-centre separation is recorded (MR-20, D03).
- [ ] 1 m straight, five times: distance and lateral error, Tribunal proposal at most 10 mm (MR-21, A13, D07).
- [ ] 90 left, 90 right and 180 degrees, five each: heading error, Tribunal proposal at most 5 degrees (MR-22, A13, D07).
- [ ] Loaded wheel PID response on the floor; INT-100 to INT-102 used raised wheels only (D07).
- [ ] Encoder width of 16 bits confirmed with the robot, including the maximum edge rate (D03, Mahesha's Tribunal item).

## 3. Line tracking (M1)

The 95% target is the user's. No success rate has been measured yet, so the first job is to get one.

- [ ] Straight crossings on the car with the final fix (INT-191): no veer after a crossing, and no false crossing on a curve from the 6 mm candidate, which saved captures cannot rule out (INT-189).
- [ ] Left IR clears black late (INT-192): swap the module and do a slow hand test, to separate the module from its mounting.
- [ ] Run 20 laps with one image and no changes, and calculate the rate with `scripts/mission_trial_score.py` (MR-26). Film them: INT-163 found telemetry cannot show where the car left the line.
- [ ] Repeat under two lighting conditions (MR-26, M1).
- [ ] Real line loss: the sweep finds the line, an exhausted search stops, and stale data does not start a blind search (MR-32, T11).
- [ ] Work through the [open issues](app_program/line_barcode/README.md#open-issues): junction on three curves, `NAV_ONSET_SKEW_MM`, pivot hand-over.
- [ ] Mounted IR levels, dead zone and left-sensor span at course lighting (MR-12 as revised for DOUT, D04, A3).
- [ ] Check whether the diagnostic traces affect tracking: compare the same number of laps on `TRACE=1` and `TRACE=0` images. The build option exists; no laps have been run.
- [ ] Change one value per trial and record each as a new INT entry.

## 4. Barcode and junctions (M2, M3)

- [ ] Action once at the junction for each of left, right, straight and U-turn at variable spacing, never undone (MR-28, D02). INT-153 stopped with a queued turn.
- [ ] Print all 26 letters and run each at course speed, in both directions (MR-27, M2). Host tests cover the patterns only.
- [ ] Flipped print on the car. All 26 letters decode in both directions on the host. C and P can complete with an unknown orientation, but Mission does not use orientation.
- [ ] U-turn: confirm the stationary pause and return-line tracking. The pause is a 100 ms trial value; about 200 ms was requested (D02).
- [ ] No queued turn: crosses straight and resumes tracking (MR-28).
- [ ] Record reads against passes as a rate. A corrupted last guard still loses the read (INT-155, third pass).
- [ ] Barcode sensor level over black and white on the mounted robot (MR-13, D04).
- [ ] Issued print framing, quiet zones and tolerances (D01, A8).
- [ ] Inverted print (white bars on black) will not read. Default: unsupported unless the course has one.
- [ ] Barcode capture ring and event queue: peak use and overflow at course speed (T04).

## 5. Obstacle and bypass (M6 to M9)

Wheels stay off for the bench steps. Bypass has never run on the car.

- [ ] S5-U10 with `./flash.sh test`: servo centre and limits, with +/-60 degrees unconfirmed (MR-18, D05).
- [ ] Sonar against a board at 100, 250, 500 and 1000 mm, plus open space; MR-17 is still PENDING without a saved measurement (A5, D05).
- [ ] Boxes of 100, 150 and 200 mm: check the `[SCAN]` rays and profile against a tape. S5-U11 used 150 and 220 mm, with mean width error of about 18 mm and no agreed target (M7).
- [ ] Both sides blocked: the robot stops rather than bypasses (MR-31).
- [ ] Bypass runs with both sides open and one restricted: no collision, larger-gap side chosen, swept envelope confirmed (MR-30, M8, D03).
- [ ] Rejoin the original line and continue (MR-30, M9, A9).
- [ ] Depth measured against a tape, and the rear edge seen while bypassing (M7).
- [ ] Record the stop reason for `SCAN` then `STOPPED` if it recurs. INT-164 suggests the car was lifted, and INT-186 confirms the saved capture cannot prove it.
- [ ] Confirm on the car that scans start only after fresh stationary feedback, with timestamped logs (T09). Host logic passes.
- [ ] Cancel a scan and restore the servo to centre on a stop, on the car, with no-echo cases (Gap 6). Mission and scan-guard host cases pass.
- [ ] Measure bypass speeds, settle time and travel budgets (D05, D07).

## 6. Hump (M4, M5)

Flash the current image, let the car sit still on the track for a second, then press GP20.

- [ ] Cross the 10 mm hump at least 3 times with barcode turns in the lap. Pass: `humps=` matches the crossings exactly, and pivots add nothing (INT-158 bug, M4).
- [ ] Reference humps from the user-selected 10 mm minimum up to the 30 mm limit, three passes each, reported against actual height (MR-29, M5, A4, D06). `HUMP_CALIB_FACTOR` 0.8 comes from one hump height.
- [ ] Agree a height error limit; "a few millimetres" is not an accepted target (A10, D10, Gap 7).
- [ ] Turn accuracy with one wheel on a hump (MR-23, M3).
- [ ] Hump on a curve, and a hump just before a barcode.
- [ ] Reverse direction over a hump: the side and roll sign are untested (D11).
- [ ] Revisit `HUMP_MIN_TRAVEL_MM` 50: it rests on only two crossings.
- [ ] Detector stuck on: current code ends an encounter at `HUMP_MAX_TRAVEL_MM`, but a real hump inside that travel can be missed (INT-171). Check on the car.
- [ ] Mounted calibration success and failure, with IMU disconnected (MR-11, D06).
- [ ] Raw IMU samples over a measured hump with the `TERRAIN_DIAGNOSTICS=1` image (S4-M01).
- [ ] Labelled traces for stationary, acceleration, turn, climb, descent and impact (T06). No turn-rate monitor exists; the GY-511 has no gyroscope.

## 7. Telemetry and mapping (M10)

- [ ] Pico telemetry at about 500 ms in the current binary format, saved (MR-08, S1-M01, Gap 5).
- [ ] Start refused without a mapping (MR-09).
- [ ] Mapping accepted while stopped, BUSY while following, cleared on reboot (MR-10, A8).
- [ ] Stop the broker mid-run: robot keeps driving, loss is shown, and recovery time is recorded (MR-33, A1, T13).
- [ ] Malformed, truncated and prohibited requests on the real link (T12, Gap 5). Host tests only so far.
- [ ] All six fault bytes under controlled fault injection (S1-M01).
- [ ] Rebuild the route from saved events (MR-38).
- [ ] Supply the professor's buffer wording (D09); it is not in this repository, so bounded rings stay.

## 8. Faults seen and not explained

- [ ] Encoder faults stopped earlier runs, and those captures are not held. `WHEEL` frames in INT-154 to 157 show no glitch (INT-165). Watch for the fault again on the car, and for `encoder_ok=0` in `HUMP` frames.
- [ ] `IMU calibration failed`: likely the car moved during boot (INT-166). Confirm by keeping it still, as in MR-11.
- [ ] `barcode capture overflow` is frequent (INT-170). Compare it with read rate on the car; change the ring only if reads fail.
- [ ] One `EVENT BARCODE` per read: host test added (INT-167). Confirm on a real run.

## 9. Full mission and repeatability

- [ ] Agree first-cut limits for lateral, turn, height, width and recovery error before any physical PASS (D10).
- [ ] Mission-day routine: fresh boot, broker, unfamiliar letters, local start, with no recompile (MR-35).
- [ ] Full course with line, barcodes, junctions, humps and an obstacle (MR-36, T16).
- [ ] Two more runs with no code or constant changes, for three in a row (MR-37). The brief assesses repeatability directly.
- [ ] Same junction revisited, repeated feature cycles, WiFi interrupted, GP21 ends the session (MR-40, endless operation).
- [ ] Run time compared with the roughly five-minute expectation (MR-36).

## 10. Week 13 evidence (minimum-bit, timing, resources)

- [ ] Timing on the final image under combined load: WCET, jitter, missed deadlines and sensor-to-motor latency (MR-39, T15, A11, D13). This is the evidence that timing has not degraded.
- [ ] No lost functionality: rerun the applicable set on the final commit (T15, T16).
- [ ] RAM usage of the final image. Latest build is in INT-174; SYS-11 and SYS-12 are October builds.
- [ ] Stack use and peak RAM measured (R3, D13). Never measured.
- [ ] Data table signed off by each owner; most sections still say "proposed" (Gap 1, R2, issue #28).
- [ ] Fault-byte owner confirmations (#113).
- [ ] Height scale and rounding confirmed by reference humps; 16-bit working and 7-bit reported widths are in code (D06, Darrius's Tribunal item).
- [ ] Packed-data race checks on the Pico under load, including lock duration (R1, R4, Gap 2). Host stress only so far.
- [ ] Measure CPU cost of packing and unpacking (Tribunal resource review).
- [ ] Stale and frozen producers, expired commands and full buffers on the Pico (T14). Host cases exist in `test_safety`, `test_motion_control` and `test_comms_report`.

### Bit reduction candidates

Built on `chore/centralise-pending`, not on `main`, with host tests and builds only (INT-178). Each needs a car run after the demonstration.

| Cut | Where | Result | Check on the car |
| --- | --- | --- | --- |
| Done | Barcode edge ring | 64 x 48 to 64 x 33 bits | Barcodes still read at course speed |
| Done | Encoder counts and origins | 32 to 16 bits each | Turns, distances and bypass travel |
| Done | `WheelPid` | 2 x 64 to 2 x 32 bits, no stored float | Loaded wheel speed against the old image |
| Done | `ObstacleRay`, `ObstacleProfile` | 32 to 24 bits; 112 to 72 bits; capped at 1023 mm | Scan rays, profile and side choice |
| Done | `IrTracker`, `LineIrSnapshot` | 160 to 64 bits; 192 to 96 bits | Line following and junctions |
| Done | Telemetry width and depth | 23 to 22 bytes | Dashboard shows both on a real link |
| Not done | `TRACE=0` as the default | Would save 3712 bits | Needs the A/B laps; removes debug frames |
| Not done | 16-bit timestamps | Would save about 250 bits | Not recommended: a short timer can alias, and bypass allows 90 s (Gap 4) |

Flash grew by 1056 bytes, because packed fields take more code to read.

## 11. Reports, demonstrations and traceability

- [ ] INT-187, INT-189 and INT-191 record `7275347`+ with uncommitted changes. Only INT-191 matches a commit (`526e5b5`); the final build is INT-193.
- [ ] Link every result to its exact commit, image hash, setup and limit, with reruns kept (Gap 8). These rows have no commit, and it cannot be recovered from the log: INT-39, INT-41, INT-133, INT-140b, INT-141, INT-142, INT-144, INT-150, INT-152, INT-159.
- [ ] Record videos and fill the index; nothing is recorded yet ([video](video/README.md)).
- [ ] Integrated demonstration (Team deliverable).
- [ ] Telemetry demonstration (Comms deliverable).
- [ ] Motion accuracy evaluation, from MR-21 to MR-23.
- [ ] PID tuning report: [draft](app_program/motion/README.md#pid-tuning-report-draft) written; needs loaded data.
- [ ] Terrain analysis report: [draft](app_program/terrain/README.md#terrain-analysis-draft) written; needs reference humps and traces.
- [ ] Update the mission-readiness checklist as items pass.

## 12. Stretch goal

Nothing open. Magnetometer fusion is skipped by default because it is optional and cannot be verified without the car. Say if you want it.


## 13. Housekeeping

- [ ] Diagnostic frames (types 9, 10 and 11): the Comms README says to keep them, but also says type 9 is not agreed. See [decisions](#decisions-for-nicholas).
- [ ] `testing/pure/test_check_barr.py:114` fails on Windows (S4-U08), so the host suite stops there on that machine. It passes on macOS; a Windows machine is needed to check a fix.
- [ ] Remove the stray `115200` file from the main checkout; it is untracked and holds only an error message from a terminal command.
- [ ] Keep this file as the completion record, closing each item only with evidence or a stated decision.

## Complete

Software items are complete only as software; physical acceptance is listed above where it applies.

- [x] Uses micro T-Kernel, not FreeRTOS.
- [x] Only Motion writes the motors.
- [x] Networking cannot start, stop or drive the robot.
- [x] Course speed fixed at 100 mm/s by a compile-time check.
- [x] JSON replaced with binary frames in code and host tests; the real link is S1-M01 above.
- [x] Message sizes fixed and asserted: 22, 11, 5, 3 and 2 bytes on `chore/centralise-pending`; `main` still sends 23.
- [x] Fault enums replaced with independent bits (Nicholas's Tribunal item).
- [x] One encoded value for each state field.
- [x] Scan state uses 3 bits (Jia Jing's Tribunal item).
- [x] Accessor functions for packed fields.
- [x] Mutex-protected snapshots written, with 5 million host reads and none torn (Shaun's Tribunal item); on-target check is in section 10.
- [x] Wire validation of length, version, type, reserved bits and ranges, in host tests.
- [x] Comms API, telemetry framework and frame documentation.
- [x] RTOS interfaces kept between subsystems.
- [x] `scripts/dashboard.html` and its test: the local edits were committed in #150.
- [x] PR 149 merged at `99fe0d8`, with its conflicts and three test-tool bugs fixed (INT-160, host only).
- [x] Merge commit `e576760` recorded for INT-132 to INT-159 (INT-161).
- [x] Watchdog code written, off by default; physical test is in section 1.
- [x] `TRACE=0` build, `scripts/mission_trial_score.py` and `scripts/triage_mqtt_capture.py` added, with host tests for the two scripts only; the laps are in section 3.
- [x] Laptop speed estimate ignores bursty packet timing, with a host test.
- [x] Integration range probe corrected to 20 bytes.
- [x] Readiness procedures updated for the digital IR sensors and reset flow, keeping MR-29 at 100 mm/s on humps.
- [x] PR 147 closed.
- [x] Overlapping logs trimmed; INT-135 and INT-136 are now 340 KB and 532 KB.
- [x] Arm builds with `WATCHDOG=0/1` and `TRACE=0/1` (INT-162, INT-174); build only.
- [x] Off-course capture analysed; no cause found, so the laps need video (INT-163).
- [x] `SCAN` then `STOPPED` analysed: likely the car being lifted, not proven (INT-164).
- [x] No echo never stops a run, in host tests (INT-169).
- [x] Event drops stay debugger-only, by default decision; no telemetry fields added.
- [x] M1 success measure agreed: one lap with correct barcodes, navigation and hump count, by Nicholas.
- [x] Obstacles on straights, by Nicholas (D12); curve options are in the [Obstacle guide](app_program/obstacle/README.md#curves-near-an-obstacle).
- [x] Restarts: loose battery connection, hardware fix, by Nicholas (INT-175).
- [x] Issue #117 stays closed; this file holds its open work, by Nicholas.
- [x] PR 149 is merged; no branch action needed.
- [x] WiFi and broker: own network and laptop broker, by Nicholas (D09). For another network, change `config/wifi_secrets.h` and rebuild.
- [x] One barcode event rule with an A-Z bound and host tests, replacing two versions (INT-193); real run is in section 8.
- [x] Watchdog feed rule needs both tasks fresh and advancing, host tested (PR 151); bench test is in section 1.
- [x] Host tests run on GitHub for PRs and `main` (PR 151). This branch has not been pushed, so that run has not happened for it.
- [x] Lap scorer rejects incomplete PASS rows and needs 19 of 20; triage keeps state transitions (INT-185).
- [x] Saved captures reviewed; no steering cause can be proven from them (INT-186).
- [x] Dead terrain snapshot, unused settings and old plan checker removed (INT-193).
- [x] Week 10 issue set aside by Nicholas; a demonstration is on 2026-11-04.
- [x] Fault bytes stay at 48 bits, by Nicholas.
- [x] Core-0 audit by source review, no defect found (INT-172); load is section 10.
- [x] Data table: watchdog, float and digital IR rows; fault bytes and obstacle profile justified as proposed (INT-173).

## Superseded

Closed by a later decision, so no work remains. `docs/tribunal1.md` still shows the earlier position.

- [x] Reset encoder distance at each barcode: replaced by wrapping counters with per-consumer origins (Gap 3).
- [x] Compare RAM and message sizes before and after: dropped by the course update of 2026-10-27.
- [x] Priority hierarchy unchanged with a separate Safety task: the 2026-10-29 design removed the task, and Motion still outranks Mission.
- [x] Finish convention (A10): endless operation is settled, with no finish marker. MR-34 stays retired.
