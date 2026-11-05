# Pending work

## Contents


- [How this list is ordered](#how-this-list-is-ordered)
- [Where everything lives](#where-everything-lives)
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
3. Course features, because the demonstration scores features working. Obstacle comes before hump because the professor's clarification calls bypass and recovery central, and bypass has only observed runs, with no measurements.
4. Whole-mission repeatability, then required Week 13 evidence.
5. Reports, stretch goals and housekeeping.

IDs: `M` requirement, `MR` readiness check, `D` pending decision, `T` integrated trial, `A` Tribunal assumption, `R` Tribunal risk, `Gap` Tribunal Version C. `docs/tribunal1.md` is a reference only, and differences from it are listed under [Superseded](#superseded).

## Where everything lives

Every pull request runs four CI checks: BARR-C:2018 on changed lines, subsystem deliverables and alignment, host tests with dashboard checks, and firmware builds for every profile. They cannot run on the car; deliverables that need it print NEEDS EVIDENCE.

All of this work is merged into `main` (PR 152, 2026-11-05). Nicholas reports two perfect full-course runs, with everything tested except the hump (INT-204, observed only). The saved log does not show those runs in full and has no barcode reads, so this is not a recorded physical pass. The hump is **untested on the car**. To return to the previous working image, use commit `7275347`.

Working rules: change one value per trial and record each as a new INT row, and close an item only with evidence or a stated decision.

## Where things stand

| Area | State | Evidence |
| --- | --- | --- |
| Firmware build | All eight images and the IMU diagnostic image build and the host suite passes. Not a firmware or physical pass. | INT-193 to INT-197, INT-205 |
| Safety and electrical | No recorded result. Wiring is user-confirmed only. | MR-01 to MR-05 |
| Watchdog | Code merged, off by default, never run on a Pico. No reset policy agreed. | `99fe0d8`, D15 |
| Line tracking | PI steering and the straight-crossing fix were reported good in two full runs, observed only. Nicholas self-tests the lap rate. | INT-200, INT-204 |
| Barcode | User reports fully functional. All letters and a flipped print not tested. | User observation, INT-156 |
| Junctions | Some turns passed; junction failures also logged. | INT-137, INT-139b, INT-153 |
| Obstacle | Scans ran on real boxes with a measured width error. Bypass ran on the car with good runs reported, but gaps are not measured and one earlier run took the farther edge, cause unknown. | S5-U10, S5-U11, INT-198, INT-201, INT-203, INT-204 |
| Hump | Latest fix built, never run on the car. Last run counted 5 for 1 crossing. | INT-158, host and build only |
| Telemetry | Binary link in use on the bench. Physical subscription check pending. | S1-M01 |
| Full mission | Two perfect runs reported without the hump (INT-204, observed only). Never run as an acceptance check. | MR-35 to MR-40 |
| Timing and resources | Not measured on the final build. | SYS-11, SYS-12 (October builds) |

## What needs the robot

Software investigation, fixes, host tests, Arm builds, static RAM/layout analysis,
protocol fixtures, report structure and procedure preparation can happen now.
They cannot close these physical items:

| Blocked acceptance | Why the robot or actual environment is needed | Checklist coverage |
| --- | --- | --- |
| Electrical and stopping safety | Continuity, Echo voltage, power rails under load, braking distance/output waveforms, impact, held-button/reset and watchdog behavior need hardware measurements. The loose-battery restart cause is already explained. | Section 1; MR-01–07, MR-16, MR-24/25, SYS-06 |
| Motion and encoder calibration | Actual wheel travel/sign, edge errors/rates, geometry, floor PID, slip and turn accuracy cannot be inferred from raised-wheel or host tests. | Section 2; MR-14/15, MR-19–23 |
| Robust line/barcode/junction behavior | Mounted sensor levels, lighting, issued prints, physical overflow rate, three curves, action hand-over and TRACE effect require the course. | Sections 3–4, 8; MR-12/13, MR-26–28/32 |
| Hump accuracy and reliability | Current fix needs actual crossings; height scale/error, reverse/curve/pivot effects, minimum detectable hump, stuck detection and disconnected-IMU behavior require measured references. | Section 6; MR-11/16/23/29 |
| Obstacle and full bypass | Servo endpoints, sonar error, swept envelope, width, blocked sides and original-line rejoin have no accepted integrated physical evidence. | Section 5; MR-17/18/30/31 |
| End-to-end telemetry | Laptop fixtures can prove codec behavior, not Pico radio timing, reconnect, real mapping/fault injection. | Section 7; MR-08–10/33/38 |
| Loaded resources and repeatability | WCET/jitter/ISR/lock cost, live stack/peak RAM, actual queue high-water marks, three full runs and physical videos require the running final image. | Sections 9–11; MR-35–40 |

## Required coverage

`docs/tribunal1.md` has no table labelled FR/NFR. Until Nicholas identifies another
source, this plan conservatively retains all M1–M10 from the owning requirements
guide plus the Tribunal's functional, safety and resource obligations. These
labels are a crosswalk, not invented instructor requirement IDs.

| Requirement / obligation | Plan coverage and closure |
| --- | --- |
| M1 line tracking and lighting tolerance | sections 2–3; qualifying lap, self-tested by Nicholas |
| M2 A–Z in either direction, stored until next junction | section 4; both traversal directions, one action/event per read, physical print coverage |
| M3 all four encoder actions, including one wheel on hump | sections 2, 4 and 6; ground distance/angle measurements and navigation trials |
| M4/M5 detection, each height/count and retained maximum | section 6; reference measurements, exact crossing count, no pivot false positives |
| M6/M7 obstacle detection, profile and clearance | section 5; measured boards/boxes and side/rear-edge evidence |
| M8/M9 safe bypass and original-line continuation | section 5; both sides, restricted/blocked cases, envelope and correct rejoin. Rejoining during BYPASS/NAVIGATE is required; a standalone REACQUIRE during the qualifying lap fails it. |
| M10 two-way MQTT, runtime mapping, readable telemetry and route evidence | section 7; malformed requests, BUSY/reboot, fault visibility and reconnect |
| Autonomous, undisclosed route, endless operation, micro T-Kernel | sections 9/11; unfamiliar runtime letters, repeated encounters, no remote driving or hard-coded route |
| Single motor writer, stop precedence, freshness, reset and fault independence | sections 1/8/10; separate intentional fault/stop tests; networking has no movement authority |
| Race-free minimum-bit data, precision, invalid/overflow behavior | section 10 and Tribunal Gaps 1–7; field census, accessor/boundary checks, final sizes and on-target timing |
| Electrical, timing, stack, performance and repeatability | sections 1/9/10 and Tribunal risks R1–R5; measured values, not host inference |
| Reports, demonstrations and traceability | section 11 and Tribunal Gap 8; unique result IDs, exact source/image, saved measurements/video links |

## Decisions for Nicholas

Open decisions sit with their items, and all go to Nicholas: print framing (D01, section 4), remaining error limits (D10, section 9) and the data table sign-off (section 10). Proposed defaults are recorded as proposed, not as sign-off.


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
- [ ] Stall guard: on for the full-course image (INT-176). Confirm on the car that it causes no false stop, and that a held wheel stops the run (MR-25, D07).
- [ ] Watchdog (D15, decided): test a `WATCHDOG=1 TEST=1` image with wheels raised: reset on a stopped task, motor outputs through reset, no restart without GP20, and no false reset under WiFi load (SYS-06). Then turn it on for `full_course`. Do not enable it on a driving car until this passes.
- [ ] Impact: tap test gives `Terrain: impact` and a latched stop (MR-16, D06).

## 2. Motion calibration

Turns, distances and barcode widths all come from encoder travel.

- [ ] Each motor drives forward on a short command (MR-19, D03).
- [ ] Encoder counts per revolution, both wheels, repeated across speeds and reversals (MR-14, A2, D03). Only manual trials so far.
- [ ] Forced encoder fault appears and clears (MR-15).
- [ ] Measure wheel diameter and effective track width; only wheel-centre separation is recorded (MR-20, D03).
- [ ] 1 m straight, five times: distance and lateral error, lateral limit at most 10 mm (MR-21, A13, D07). Run on the final image, because stop points depend on how closely the wheels follow the profile.
- [ ] 90 left, 90 right and 180 degrees, five each: heading error, limit at most 5 degrees (MR-22, A13, D07). Run on the final image, for the same reason.
- [ ] Loaded wheel PID response on the floor; INT-100 to INT-102 used raised wheels only (D07).
- [ ] Encoder width of 16 bits confirmed with the robot, including the maximum edge rate (D03, Mahesha's Tribunal item).

## 3. Line tracking (M1)

Nicholas self-tests the lap rate (2026-11-05), so no recorded success rate is required.

- [ ] Left IR clears black late (INT-192): swap the module and do a slow hand test, to separate the module from its mounting.
- [ ] Real line loss: the sweep finds the line, an exhausted search stops, and stale data does not start a blind search (MR-32, T11).
- [ ] Decide wider IR spacing (Nicholas): measure spot spacing and line width after any move, then re-measure `NAV_ONSET_SKEW_MM` and `LINE_LOST_MM`. At the INT-98 33 mm, 23 mm line, geometry allows a curve to read both black from 46 degrees; 60 mm raises that to 68 degrees and the white/white band from 10 to 37 mm.
- [ ] Mounted IR levels, dead zone and left-sensor span at course lighting (MR-12 as revised for DOUT, D04, A3).
- [ ] Straight crossings roll through at course speed with no stop or pause: holds straight across the bar, regains the line, no veer afterwards, and no false crossing on a curve from the 6 mm candidate, which saved captures cannot rule out (MR-28, INT-189, INT-191). Heading drift was the reason for the earlier stop, so film it; lengthen or shorten `NAV_STRAIGHT_MM` one trial at a time if needed.
- [ ] Junction advance on a turn: the car rests where the distance says, with the axle over the crossing centre, now that it slows on the profile. A turn too late to stop there is rejected (`nav_rejected=2`).
- [ ] Smoothness on film and `WHEEL` traces at 100 mm/s, on the merged PI steering (reported good, INT-200, no evidence saved). Moving off and slowing down like a lift, with no wheels sitting still then jumping (tune `MOTION_ACCEL_MMPS2`, `MOTION_JERK_MMPS3`, `MOTION_KS`); no straight-line jolt at the lowered 300 and 600 Q/s steering rates; tightest curve and S-bend hold (`LINE_KP_Q`, `LINE_KI_QPMM`, `LINE_I_MAX_Q`, `LINE_I_FADE_QPMM` are provisional); pivot ends not abrupt, or try a slower final part and recheck the landing point. If not very smooth, lower `COURSE_SPEED_MMPS`; speed is the second priority. Below 60 mm/s needs a new feedforward fit.

## 4. Barcode and junctions (M2, M3)

- [ ] Action once at the junction for each of left, right, straight and U-turn at variable spacing, never undone (MR-28, D02). INT-153 stopped with a queued turn.
- [ ] Print all 26 letters and run each at course speed, in both directions (MR-27, M2). Host tests cover the patterns only.
- [ ] Flipped print on the car, turned 180 degrees and read from either end (Nicholas: must read). All 26 letters decode in both directions on the host. C and P can complete with an unknown orientation, but Mission does not use orientation.
- [ ] U-turn: confirm the stationary pause and return-line tracking. The pause is now 200 ms in code, unmeasured (D02).
- [ ] No queued turn: crosses straight and resumes tracking (MR-28).
- [ ] Record reads against passes as a rate, with one `EVENT BARCODE` per read (INT-167). A corrupted last guard still loses the read (INT-155, third pass).
- [ ] Barcode sensor level over black and white on the mounted robot (MR-13, D04).
- [ ] Issued print framing, quiet zones and tolerances (D01, A8).

## 5. Obstacle and bypass (M6 to M9)

Wheels stay off for the bench steps. Bypass has run on the car, observed only (INT-203, INT-204).

Known limit: a real object at about twice the box range, right beside its edge, is ignored by the room check as a ghost echo; only the forward check on the leg out and the side check while passing would catch it (INT-201). If `SCAN` then `STOPPED` recurs, record the stop reason: INT-164 suggests the car was lifted, and INT-186 confirms the saved capture cannot prove it.

- [ ] S5-U10 with `./flash.sh test`: servo centre and limits, with +/-60 degrees unconfirmed (MR-18, D05).
- [ ] Sonar against a board at 100, 250, 500 and 1000 mm, plus open space; MR-17 is still PENDING without a saved measurement (A5, D05).
- [ ] Boxes of 100, 150 and 200 mm: check the `[SCAN]` rays and profile against a tape. S5-U11 used 150 and 220 mm, with mean width error of about 18 mm and no agreed target (M7).
- [ ] Both sides blocked: the robot stops rather than bypasses (MR-31).
- [ ] Bypass at 100 mm/s, including its reverse along the approach when too close to pivot, not yet confirmed on the car: start with a wheels-raised check, then the bench, and stop if the side ray misses the box edge (D07).
- [ ] Bypass runs with both sides open and one restricted: no collision, side nearer the line's centre chosen, 90 degree legs clear the box, swept envelope confirmed, rejoins the original line and continues. Record bypass speeds, settle time and travel budgets (MR-30, M8, M9, A9, D03, D05, D07).
- [ ] Side too close while passing: the car steps out 40 mm and carries on. Both the 40 mm step and the 40 mm passing allowance are provisional; count step-outs on real runs, which should be none on a normal pass, and a false reading such as the car's own body would use up the room and then stop (D07).
- [ ] Rear clearance when pivoting back: tape the gap to the box, and compare the pass with the real box. `BYPASS_MIN_DEPTH_MM` is provisional at 200 mm, set it to the deepest box, and the course obstacle depth is not specified (D02).
- [ ] Obstacle stop on the car: against a board, the car starts slowing at 191 mm and rests at 150 mm. Measure the rest distance with a tape; sonar noise or wheel lag will move it (D16).
- [ ] Confirm on the car that scans start only after fresh stationary feedback, with timestamped logs (T09). Host logic passes.
- [ ] Cancel a scan and restore the servo to centre on a stop, on the car, with no-echo cases (Gap 6). Mission and scan-guard host cases pass.

## 6. Hump (M4, M5)

Flash the current image, let the car sit still on the track for a second, then press GP20.

- [ ] Cross the 10 mm hump at least 3 times with barcode turns in the lap. Pass: `humps=` matches the crossings exactly, and pivots add nothing (INT-158 bug, M4).
- [ ] Reference humps from the user-selected 10 mm minimum up to the 30 mm limit, three passes each, reported against actual height (MR-29, M5, A4, D06). `HUMP_CALIB_FACTOR` 0.8 comes from one hump height. This also confirms height scale and rounding; 16-bit working and 7-bit reported widths are in code (Darrius's Tribunal item).
- [ ] Turn accuracy with one wheel on a hump (MR-23, M3).
- [ ] Hump on a curve, and a hump just before a barcode.
- [ ] Reverse direction over a hump: the side and roll sign are untested (D11).
- [ ] Revisit `HUMP_MIN_TRAVEL_MM` 50: it rests on only two crossings.
- [ ] Detector stuck on: current code ends an encounter at `HUMP_MAX_TRAVEL_MM`, but a real hump inside that travel can be missed (INT-171). Check on the car.
- [ ] Mounted calibration success and failure, with IMU disconnected (MR-11, D06). Keep it still to confirm `IMU calibration failed` came from the car moving during boot (INT-166).
- [ ] Raw IMU samples over a measured hump with the `TERRAIN_DIAGNOSTICS=1` image (S4-M01).
- [ ] Labelled traces for stationary, acceleration, turn, climb, descent and impact (T06). No turn-rate monitor exists; the GY-511 has no gyroscope.
- [ ] Turn-rate monitoring is a listed Terrain deliverable and does not exist. Build it from the encoders and label it so, or record a decision to leave it out. Writing it needs no car.

## 7. Telemetry and mapping (M10)

- [ ] Dashboard IMU graphs on the car: tilt, XYZ and terrain profile against a measured hump. HUMP frames now run on the full-course image, so check they do not disturb line tracking or other telemetry.

- [ ] Pico telemetry at about 500 ms in the current binary format, saved (MR-08, S1-M01, Gap 5).
- [ ] Start refused without a mapping (MR-09).
- [ ] Mapping accepted while stopped, BUSY while following, cleared on reboot (MR-10, A8).
- [ ] Stop the broker mid-run: robot keeps driving, loss is shown, and recovery time is recorded (MR-33, A1, T13).
- [ ] Malformed, truncated and prohibited requests on the real link (T12, Gap 5). Host tests only so far.
- [ ] All six fault bytes under controlled fault injection (S1-M01).
- [ ] Rebuild the route from saved events (MR-38).

## 8. Faults seen and not explained

- [ ] Encoder faults stopped earlier runs, and those captures are not held. `WHEEL` frames in INT-154 to 157 show no glitch (INT-165). Watch for the fault again on the car, and for `encoder_ok=0` in `HUMP` frames.
- [ ] `barcode capture overflow` is frequent (INT-170). Measure peak capture ring and event queue use at course speed and compare with read rate (T04); change the ring only if reads fail.

## 9. Full mission and repeatability

- [ ] Agree the remaining error limits before any physical PASS: height, width, measured speed and recovery (D10). Lateral and turn limits are decided.
- [ ] Mission-day routine: fresh boot, broker, unfamiliar letters, local start, with no recompile (MR-35).
- [ ] Full course with line, barcodes, junctions, humps and an obstacle (MR-36, T16). Two runs reported without the hump (INT-204); still needs the hump and a saved log showing barcodes.
- [ ] Two more runs with no code or constant changes, for three in a row (MR-37). The brief assesses repeatability directly.
- [ ] Same junction revisited, repeated feature cycles, WiFi interrupted, GP21 ends the session (MR-40, endless operation).
- [ ] Run time compared with the roughly five-minute expectation (MR-36).

## 10. Week 13 evidence (minimum-bit, timing, resources)

- [ ] Timing on the final image under combined load: WCET, jitter, missed deadlines and sensor-to-motor latency (MR-39, T15, A11, D13). This is the evidence that timing has not degraded.
- [ ] No lost functionality: rerun the applicable set on the final commit (T15, T16).
- [ ] RAM usage of the final image. Latest build is in INT-174; SYS-11 and SYS-12 are October builds.
- [ ] Stack use and peak RAM measured (R3, D13). Never measured.
- [ ] Nicholas reviews the data table; most sections still say "proposed", and each [bit reduction](#bit-reduction-candidates) group needs sign-off after its car check (Gap 1, R2).
- [ ] Nicholas confirms fault meanings and simultaneous-fault coverage (former #113).
- [ ] Packed-data race checks on the Pico under load, including lock duration (R1, R4, Gap 2). Host stress only so far.
- [ ] Measure CPU cost of packing and unpacking (Tribunal resource review).
- [ ] Stale and frozen producers, expired commands and full buffers on the Pico (T14). Host cases exist in `test_safety`, `test_motion_control` and `test_comms_report`.

### Bit reduction candidates

Merged with host tests and builds only (INT-178). Each needs a car run.

| Cut | Where | Result | Check on the car |
| --- | --- | --- | --- |
| Done | Barcode edge ring | 64 x 48 to 64 x 33 bits | Barcodes still read at course speed |
| Done | Encoder counts and origins | 32 to 16 bits each | Turns, distances and bypass travel |
| Done | `WheelPid` | 2 x 64 to 2 x 32 bits, no stored float | Loaded wheel speed against the old image |
| Done | `ObstacleRay`, `ObstacleProfile` | 32 to 24 bits; 112 to 72 bits; capped at 1023 mm | Scan rays, profile and side choice |
| Done | `IrTracker`, `LineIrSnapshot` | 160 to 64 bits; 192 to 96 bits | Line following and junctions |
| Done | Telemetry width, with clearance in place of depth | 23 to 22 bytes | Dashboard shows width and clearance on a real link |
| Done | `TRACE=0` in `full_course` | 464 bytes less RAM, 2304 bytes less flash (INT-194) | Trial and course images drive alike |
| Not done | 16-bit timestamps | Would save about 250 bits | Not recommended: a short timer can alias, and bypass allows 90 s (Gap 4) |

Flash grew by 1056 bytes, because packed fields take more code to read.

## 11. Reports, demonstrations and traceability

- [ ] Record videos and fill the index; nothing is recorded yet ([video](video/README.md)).
- [ ] Integrated demonstration (Team deliverable).
- [ ] Telemetry demonstration (Comms deliverable).
- [ ] Motion accuracy evaluation, from MR-21 to MR-23.
- [ ] PID tuning report: [draft](app_program/motion/README.md#pid-tuning-report-draft) written; needs loaded data.
- [ ] Terrain analysis report: [draft](app_program/terrain/README.md#terrain-analysis-draft) written; needs reference humps and traces.
- [ ] Update the mission-readiness checklist as items pass.
- [ ] Final submission docs are only `README.md` and `team.md` (Nicholas, 2026-11-05). As a last step, move what must survive out of `docs/project-specs.md`, `docs/architecture.md` and `docs/tribunal1.md`, then remove them and fix every link, `AGENTS.md` and `scripts/check_deliverables.py`, which read those files.
- [ ] Each owner can explain any line of their submitted code; failure attracts a penalty. Contribution evidence (GitHub commits and merges) can also change individual grades.

## 12. Stretch goal

Nothing open. Magnetometer fusion is skipped by default because it is optional and cannot be verified without the car. Say if you want it.


## 13. Housekeeping

- [ ] Save the bypass and full-course logs as evidence: `run1.log` to `run3.log` in `testing/integration/logs/2026-11-05_INT-198/` (INT-198, INT-204) are only in the main checkout, about 23 MB, so trim or compress them first. The empty `2026-11-05_line-pi-smooth/run1.log` can go.
- [ ] Decide what to say about the legacy BARR-C:2018 findings: about 1,530 remain, and CI checks changed lines only. Code quality is assessed.

## Complete

Software items are complete only as software; physical acceptance is listed above where it applies.

- [x] Uses micro T-Kernel, not FreeRTOS.
- [x] Only Motion writes the motors.
- [x] Networking cannot start, stop or drive the robot.
- [x] One course speed, `COURSE_SPEED_MMPS`, 100 mm/s today, used by following, junction advances and bypass (INT-194). Lowering it needs one line.
- [x] JSON replaced with binary frames in code and host tests; the real link is S1-M01 above.
- [x] Message sizes fixed and asserted: 22, 11, 5, 3 and 2 bytes.
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
- [x] `TRACE=0` build, `scripts/mission_trial_score.py` and `scripts/triage_mqtt_capture.py` added, with host tests for the two scripts only; Nicholas self-tests the laps.
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
- [x] Network: the instructor's, by Nicholas (D09); only `config/wifi_secrets.h` changes.
- [x] One barcode event rule with an A-Z bound and host tests, replacing two versions (INT-193); real run is in section 8.
- [x] Watchdog feed rule needs both tasks fresh and advancing, host tested (PR 151); bench test is in section 1.
- [x] Host tests run on GitHub for PRs and `main` (PR 151).
- [x] Lap scorer rejects incomplete PASS rows and needs 19 of 20; triage keeps state transitions (INT-185).
- [x] Saved captures reviewed; no steering cause can be proven from them (INT-186).
- [x] Dead terrain snapshot, unused settings and old plan checker removed (INT-193).
- [x] Straight crossings roll through without stopping, by Nicholas (INT-197, host and build only).
- [x] Lift-style profile for every planned move, with obstacle stops slowing on it; safety stops stay immediate (INT-195, host and build only).
- [x] `motion_stop_distance_mm` tells other subsystems the travel of a planned stop. Obstacle slowing starts at 191 mm on the current profile to rest at 150 mm, the bypass rests at the rear of the box, and late junction turns are rejected (INT-196, host and build only).
- [x] Watchdog policy: on for `full_course` after the SYS-06 bench test passes, by Nicholas (D15).
- [x] Diagnostics only in test images: `full_course` is `TRACE=0`, `./flash.sh trial` drives with them, by Nicholas (INT-194).
- [x] Accuracy limits of 10 mm lateral and 5 degrees heading, by Nicholas (D10).
- [x] Colour-inverted prints out of scope; 180-degree flip required and already in the decoder, by Nicholas.
- [x] Priorities: smoothness and accuracy first, then speed, by Nicholas.
- [x] Week 10 issue set aside by Nicholas; a demonstration is on 2026-11-04.
- [x] Fault bytes stay at 48 bits, by Nicholas.
- [x] Core-0 audit by source review, no defect found (INT-172); load is section 10.
- [x] Data table: watchdog, float and digital IR rows; fault bytes and obstacle profile justified as proposed (INT-173).
- [x] Instructor network is no longer tracked as pending work, by Nicholas (2026-11-05).
- [x] Obstacle depth is left out; the car only needs clearance to bypass, by Nicholas (2026-11-05). The rear-clearance check is in section 5.
- [x] Lapping test dropped; Nicholas self-tests. The scorer stays as a tool (2026-11-05).
- [x] Professor's buffer wording was never seen, by Nicholas (2026-11-05), so bounded rings stay.

## Superseded

Closed by a later decision, so no work remains. `docs/tribunal1.md` still shows the earlier position.

- [x] Reset encoder distance at each barcode: replaced by wrapping counters with per-consumer origins (Gap 3).
- [x] Compare RAM and message sizes before and after: dropped by the course update of 2026-10-27.
- [x] Priority hierarchy unchanged with a separate Safety task: the 2026-10-29 design removed the task, and Motion still outranks Mission.
- [x] Finish convention (A10): endless operation is settled, with no finish marker. MR-34 stays retired.
