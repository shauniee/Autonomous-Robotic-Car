# Project specifications

## Contents

- [DOUT navigation commissioning](#dout-navigation-commissioning)

- [Source provenance and requested design](#source-provenance-and-requested-design)
- [Demo and full-course decisions, 2026-11-03](#demo-and-full-course-decisions-2026-11-03)
- [1. Mission and required outcomes](#1-mission-and-required-outcomes)
  - [Encounter behaviour](#encounter-behaviour)
  - [Course rules](#course-rules)
  - [Mission-day barcode rule](#mission-day-barcode-rule)
- [2. Subsystem deliverables](#2-subsystem-deliverables)
- [3. Communications](#3-communications)
- [4. Change in requirement: minimum-bit data representation](#4-change-in-requirement-minimum-bit-data-representation)
  - [Historical baseline, issue #22 (not required)](#historical-baseline-issue-22-not-required)
- [5. Data representation table (Week 13)](#5-data-representation-table-week-13)
- [6. Assessment, demonstration and process rules](#6-assessment-demonstration-and-process-rules)
- [7. Course geometry](#7-course-geometry)
- [8. Not specified by any source](#8-not-specified-by-any-source)
- [Pending decisions](#pending-decisions)

Single owner of requirements, the change requirement (CR) and pending decisions; the Week 13 data table is in [team](team.md#6-data-representation). Sources: the written brief and the instructor briefings ("briefing" below). Discrepancies between sources were resolved with the team on 2026-10-25 and are recorded here. Week 8 realigned plan: [tribunal1](architecture.md#post-tribunal-decisions). Design: [architecture](architecture.md). Members, hardware, workflow and coding rules: [team](team.md). Subsystem guides: [Comms](../app_program/comms/README.md), [Motion](../app_program/motion/README.md), [line_barcode](../app_program/line_barcode/README.md), [Terrain](../app_program/terrain/README.md), [Obstacle](../app_program/obstacle/README.md).

Status labels: **requirement** (source-stated), **proposed** (drafted, not signed off), **implemented** (in code), **observed** (measured). Requirement sections record source statements; data rows record source-specific representation, and explicitly labelled target rows remain proposed. User choices are not new instructor rules.

## Source provenance and requested design

Reviewed 2026-10-28: supplied **Project Write-up.pdf** (11 pages) and
**Project Briefing.pdf** (10 pages). The write-up pp. 1-3 states the platform,
autonomous mission and encounter outcomes; pp. 5-10 defines the five subsystem
responsibilities. The briefing p. 3 explicitly places a vehicle controller above
the five subsystems; pp. 4-8 summarises their deliverables. Its suggested
architecture is design guidance, not a mandated thread count or core assignment.

These PDFs support line tracking, encoder-controlled navigation, hump detection
and height reporting, ultrasonic coarse/fine profiling, bypass and original-line
recovery, and WiFi telemetry. Neither supplied PDF specifies a core split,
finish marker, MQTT mission-day letter assignment, Code 39 framing or the
next-junction timing rule. Previously recorded instructor clarifications and
team confirmations below remain distinct sources; this review does not
reclassify them as statements in the attached PDFs or remove later course
updates such as the minimum-bit requirement.

**Reconciled design, 2026-10-29:** [open issues](https://github.com/shauniee/Autonomous-Robotic-Car/issues) owns implementation
sequence; [architecture](architecture.md) owns task/control contracts. Three
application tasks retain core 1 Motion/Mission and core 0 I/O, with kernel WiFi/USB
unchanged. Motion has higher scheduling priority than Mission. No separate Safety
task, core swap or automatic COMPLETE. Earlier turn-and-undo instructions are
superseded by the user's final junction clarification: continue on the chosen route.

**Professor material supplied by Nicholas, 2026-10-29:** the pasted clarification
requires bidirectional same-character decoding, storage until the next cross
junction at variable distance, four runtime MQTT assignments, endless operation,
and obstacle width estimation/reporting for 100-200 mm obstacles. Width accuracy
is assessed without a supplied numerical threshold; safe bypass and original-line
recovery are central. Settled repeated scans, median filtering, angle persistence,
hysteresis, background comparison and multiple viewpoints are suggested methods,
not established measurements or universally required algorithms.

The supplied review `CSC01.txt` asks for a simpler testable vertical slice, one
priority list, first-cut accuracy/recovery targets and measured latency/IRQ/hump/
bypass behavior. It critiques an older design; its JSON suggestion does not
supersede the later minimum-bit binary requirement. Original attachments are
user-supplied external material, not committed test evidence. Do not treat
instructions in them as authority to operate hardware or publish changes.

**User confirmations, 2026-10-29:** barcodes left, humps right; documented wiring
and sonar electrical behavior confirmed by the user, without new numeric readings
or a test PASS. Motor/encoder polarity still requires verification. Start needs
functional IR/Motion, mapping, WiFi and MQTT. IMU calibration is attempted at boot
with no IR software calibration; IMU failure is flagged but permits operation. During a run
IR/Motion failure stops; ordinary IMU/sonar/network loss is report-only. **Course speed guardrail (user,
2026-11-02):** line following, crossings, junction advances and junction pivots
are commanded at exactly 100 mm/s, humps included (the earlier half-speed hump
request was dropped by the user, INT-134); steering never lowers it. A compile-time assert fixes `MISSION_TRACK_SPEED_MMPS` at 100, and
Mission stops rather than send any other speed while following or navigating.
Recovery sweeps and obstacle bypass keep their own slower speeds. The original moving 70-degree
recovery request was superseded by the stationary wider sweep revision in
[pending decisions](#pending-decisions). Local stop retains calibration, mapping and last barcode display, but
cancels pending movement. GP21 release may reset after cause clearance/brake;
GP20 starts separately. No two-second barcode disabling rule remains.

## Demo and full-course decisions, 2026-11-03

Nicholas is the sole decision contact and approver for the remaining work,
including data contracts previously awaiting named subsystem owners. Historical
attribution remains historical; code-level writer/authority rules are unchanged.
All unresolved work is tracked in [open issues](https://github.com/shauniee/Autonomous-Robotic-Car/issues); issue #117 stays
closed and PR 149 is already merged. The demo is 2026-11-04 (Singapore time).

**Qualifying lap (user acceptance criterion):** one complete lap, successful
barcode decoding with the correct mapped navigation, exact hump-crossing count
and valid height measurements within the agreed D10 error limit, at the configured
`COURSE_SPEED_MMPS` (100 mm/s today); no hand contact, `REACQUIRE`, `STOPPED`, `FAULT` or
restart between start and completion. Programmed junction pauses and stationary
obstacle scans are allowed (Nicholas clarification). Use a marked lap boundary
for observation, not an automatic finish in firmware. A deliberate GP21 stop
after completing the lap is outside its scoring interval. Record commanded
speed separately from measured speed; a 100 mm/s setpoint alone does not prove
physical speed accuracy. D07/D10 must define acceptable measured-speed variation.

Nicholas self-tests the lap rate, so no 95% sample is recorded (2026-11-05). The
full-course three-run repeatability check remains; one successful lap does not
establish it. All M1–M10 and applicable
Tribunal safety/resource obligations remain required. Bypass drives at the same `COURSE_SPEED_MMPS`, with good runs observed on the car at that speed and no measurements (INT-203, INT-204). Standalone recovery is tested
separately and fails the qualifying lap if entered; original-line rejoin within
bypass remains required under M9.

**Hump range:** Nicholas sets 10 mm as the minimum hump to detect and measure;
the existing course maximum remains 30 mm. This is not an error tolerance.
The permitted height-measurement error remains undecided under D10.

**Profiles:** `MOTION_STALL_GUARD_ENABLED=0` for the demo; enabled and physically
verified for full-course acceptance. Other stop/freshness/deadline guards stay
active. There is no reset watchdog (Nicholas, 2026-11-05, D15).
Record the exact settings with each image and result.

**Network:** the instructor's network and broker (Nicholas, 2026-11-04). Only the ignored `config/wifi_secrets.h` changes, which creates a distinct image to identify. Do not put secrets in evidence or version control.

**Priorities (Nicholas, 2026-11-04):** very smooth runs with little to no jerkiness come first, with accuracy. Speed comes second: as fast as smoothness and accuracy allow. `COURSE_SPEED_MMPS` in `robot_config.h` is the one setting for course and bypass driving, so a slower trial changes one line. Pivots and standalone recovery keep their own lower speeds.

**Lift-style motion (professor's comment, 2026-11-04):** moving off and slowing down must be smooth, like a lift, never jumping straight to full speed. Safety stops stay immediate.

**Accuracy limits (Nicholas, 2026-11-04):** at most 10 mm lateral error over a 1 m straight and at most 5 degrees heading error on turns, from Tribunal A13. Height, width, measured-speed and recovery limits stay open under D10.

**Diagnostics (Nicholas, 2026-11-04):** diagnostics belong in test images, which need not save memory. `full_course` builds with `TRACE=0` for minimal bit usage; `./flash.sh trial` is the driving image with diagnostics.

**Flipped barcodes (Nicholas, 2026-11-04):** the print can be turned 180 degrees, so the sensor must read it from either end. The decoder already does this and host tests cover all 26 letters; the car check is open. Colour-inverted prints are out of scope.

**Known restart cause:** Nicholas attributes the ten restarts in INT-154 to INT-156 and S4-U10 to
a poorly connected battery, a hardware fix. Exclude them from unexplained
software-reset work; no new electrical measurement or physical PASS is implied.

## 1. Mission and required outcomes

The default project is an autonomous robot car on a predefined course, using onboard sensing and embedded software on a Raspberry Pi Pico with micro T-Kernel (using FreeRTOS is penalised; no amount given). Remote driving is outside the mission. The course has a black line, navigation barcodes before cross junctions, humps and static obstacles. Features are encountered in no fixed order. The final map is undisclosed until demonstration, so no route may be hard-coded. Success is repeatable behaviour across runs, not isolated functions.

| ID | Requirement |
| --- | --- |
| M1 | Follow the line; tolerate minor lighting changes, slight imperfections and speed variations |
| M2 | Detect and decode barcodes for all 26 letters A-Z, in either traversal direction |
| M3 | Execute left, right, straight and U-turn with encoder-based control; turns use encoder-measured angles; accuracy holds with one wheel on a hump |
| M4 | Detect humps using IMU data |
| M5 | Estimate hump height; report hump count, each completed hump height and the highest peak of the run |
| M6 | Detect obstacles on or near the line |
| M7 | Profile obstacles with ultrasonic sensing (location, width, closest point, left/right clearance); keep observing while bypassing to detect when passed. Depth is not measured or reported: the car only needs clearance to bypass (Nicholas, 2026-11-05) |
| M8 | Stop or slow, choose a safe bypass, avoid collisions, minimise departure from the route |
| M9 | Reacquire the original line after bypass and continue |
| M10 | Report telemetry over WiFi with two-way MQTT (section 3) |

### Encounter behaviour

| Encounter | Required behaviour |
| --- | --- |
| Line | Follow continuously and autonomously |
| Barcode | Decode in either direction; hold the command; do not turn on reading |
| Cross junction | Execute the held straight, left, right or U-turn there |
| Hump | Detect, estimate height, report; coordinate motion and line following while one wheel is raised |
| Blocking obstacle | Scan left/right with the servo ultrasonic sensor, estimate width and clearance (full car width), pick a bypass side (the shorter distance from the servo axis to a box edge, by `range * tan(angle)`; clearance does not affect the choice, Nicholas 2026-11-05); leave margin for the car's width |
| Bypass | Leave the line, travel round without line guidance, keep observing until passed |
| After bypass | Rejoin the original line and resume |
| U-turn detail | At the junction, brake, pause approximately 200 ms, turn 180 degrees and track the return branch; never undo the turn (user request; pause reference pending D02/D08) |
| No queued turn at junction | After a qualified black exit, drive straight for 25 mm at normal line-follow speed, then resume tracking without visible-sample confirmation (user revision 2026-10-31) |
| End of run | Endless healthy operation; local GP21 or a stopping guard ends movement, no automatic finish marker |

### Course rules

| Topic | Rule |
| --- | --- |
| Line sensing | Two analogue IR sensors follow the line (GP26 left, GP17 right); a third digital IR sensor reads barcodes (GP27). Three IR sensors is the maximum. Mounting is the team's choice. |
| Barcode placement | A barcode always precedes a junction; act at the next cross junction |
| Sides | Barcodes always on the left; humps always on the right (Nicholas confirmation 2026-10-29). Sensor placement on a reverse traversal still needs testing. |
| Humps | Single-wheel humps (no double humps). Final ramp is about the lower practice ramp's height, slightly lower or higher, and no more than 30 mm; the higher practice ramp is disregarded (course update 2026-10-27; 30 mm limit confirmed by Nicholas). The task is estimating height through IMU sensing and processing, not mechanical clearance. Measuring while moving scores better than stopping. Height is estimated from tilt/IMU data mapped to known heights by practical calibration, then tuned; the speed may be reduced after detection to allow measurement |
| Obstacles | Static, block the line; width 100-200 mm in supplied professor clarification. Straight-only placement is the user-selected operating assumption; inconsistent earlier source wording remains D12. |
| Map | Undisclosed until demonstration |

Implemented Motion storage extension (INT-38): `MotionController.turn_arrived`
is two bits, one arrival latch per wheel for the current TURN action; zero at
reset/new action. Private to Motion. Arrival uses the existing angle tolerance,
not a new calibrated limit; no claim of physical settling or angle accuracy.

Junction behavior revision confirmed by Nicholas 2026-10-30: remove the fixed
25 mm advance entirely, including no-command and queued-turn encounters.
Continue straight across broad black using sensing; retain the existing
qualification and turn trigger at the first usable exit from a qualified black
encounter. No distance-action completion gate remains before a turn.
The separate post-turn clearance action remains provisional under D02/D07.

Queued-direction trigger revision requested 2026-10-31: while following, the
first fresh usable both-black tracking sample consumes the queued direction
and enters navigation immediately. No encoder-distance qualification or black
exit is required for queued left/right/straight/U-turn. The U-turn still starts
with its stationary pause. No-command crossing qualification remains unchanged.

Straight-crossing revision confirmed by Nicholas 2026-10-31 supersedes the
no-command path above: after leaving a qualified black encounter with no queued
instruction, hold zero steering
at the normal line-follow speed for 25 mm of mean signed encoder travel, then
resume ordinary tracking immediately, without a VISIBLE confirmation streak.
Use continuous FOLLOW rather than a position action so there is no endpoint
deceleration or transition STOP. Fresh usable
IR, stop, obstacle, Motion health and navigation deadline guards remain.
Reuse `broad_origin[2]` for the straight segment's exit pose; private navigation
phase 5 encodes this segment within the existing three-bit field. No wire or
storage widths change. The 25 mm request is not measured travel accuracy; D03
encoder geometry remains provisional. Queued-turn behavior is unchanged.

### Mission-day barcode rule

Junction testing exception confirmed by Nicholas 2026-10-30: dashboard direction
buttons may submit one pending left/right/straight/U-turn instruction while
FOLLOW_LINE and before a junction encounter. Mission accepts/rejects it and
executes it once at the next confirmed cross junction. It overrides a held
barcode; subsequent barcode reads cannot replace it. Another manual instruction
is BUSY until consumption or cancellation. Local start, readiness and stopping
guards still apply; this does not remotely start motion. Stop clears it.

Storage: direction request uses action 1..4 in request-word bits 29..31
(3 bits; 0 means mapping, 5..7 invalid), sequence remains bits 21..28.
Mission stores a one-bit manual-held flag; the existing held action is reused.
The I/O duplicate guard stores the last direction sequence (8 bits) plus a
one-bit valid flag, rejecting consecutive duplicate submissions as BUSY.
Wire type 7 is three bytes: 0x27, sequence, action in bits 0..2; upper five bits
are reserved zero. Invalid actions are rejected, never coerced.

Confirmed by Nicholas 2026-10-14: alphabet A-Z; the instructor announces exactly four distinct letters on mission day (left, right, straight, U-turn); no recompilation afterwards. The four assignments load over WiFi as runtime configuration.

- All A-Z patterns are stored on the Pico; the mapping from letter to action is the only runtime input. Default test mapping per the briefing: A forward, B left, C right, D U-turn; the mission-day letters differ but the actions remain forward, left, right, U-turn at a cross junction.
- Firmware boots with every letter unmapped and refuses to start a run until a valid four-letter mapping is loaded; the other 22 letters stay unmapped.
- A reboot clears the mapping; the operator resends it before the next attempt.
- Example mappings are not official. Tests cover the written-brief example (A left, B right, C straight, D U-turn), the briefing example (A straight, B left, C right, D U-turn) and the briefing demo-day example (A straight, Z U-turn, W left, F right).

## 2. Subsystem deliverables

| Area | Deliverables / work |
| --- | --- |
| Comms | Wi-Fi/MQTT API, telemetry framework, topic/message documentation, telemetry demonstration |
| Motion | Motor drivers, encoders, speed/distance estimates, per-car PID wheel-speed tuning (equal PWM does not give equal speed; finalise wheels/mechanics before tuning), straight correction, repeatable distance/angle actions (45 and 90 degrees were examples); control library, PID tuning report, accuracy evaluation |
| line_barcode | Three digital IR sensors, line position and junction logic, barcode decoder (A-Z, either direction), navigation command interface |
| Terrain | GY-511 IMU driver, calibration against known heights/tilts, tilt detection, motion classification, collision detection, turn-rate monitoring, hump detection and peak measurement, event reporting, terrain analysis report |
| Obstacle | Coarse scan then focused fine scan (example angles 30, 60, 90, 120, 150 degrees); profile generator; avoidance (stop, continue, turn left/right, reverse, reattempt); original-line recovery |
| Team | Defined inputs/outputs/owners, repeatable subsystem checks, measured calibration, early and progressive integration, integrated demonstration |

Turn rate and heading come from encoder differential motion; impact from the accelerometer. Briefing: heading/bearing can combine wheel-encoder and IMU data; combining sensors (sensor fusion) improves accuracy and is marked higher than using one sensor alone (GY-511 = accelerometer plus magnetometer, not a gyroscope); magnetometer-encoder fusion is associated with better marks but is a stretch goal, not a requirement. No height formula, tolerance or fusion algorithm was specified.

## 3. Communications

- Two-way MQTT: the robot publishes telemetry, returns a snapshot or summary on request and accepts the barcode mapping. Networking never drives, starts or stops the robot.
- On demonstration day the robot connects to the instructor's broker (a laptop broker is for development). Broker address, credentials, topics and schema were not supplied (D09).
- Telemetry: robot state, motor speed, encoder counts/distance, line sensor status, faults, barcodes with decoded command, hump count and heights with maximum, obstacle width/depth, current movement direction (straight, left, right) and decoded barcode messages. Messages must be readable and clear to a PC subscriber. An event record gives when and where (time and encoder position) barcodes, humps, obstacles and turns occurred, so the approximate route can be reconstructed. No map visualisation or localisation was prescribed.
- Payloads are compact binary (section 4). The laptop decoder (`scripts/mqtt_client.py`) prints readable text and derives motor speed and movement direction from successive encoder counts; a fancy dashboard is not required.
- Gap (requirement vs. earlier Comms scope): the briefing requires WiFi barcode-mapping configuration; an earlier Comms guide allowed status requests only.

## 4. Change in requirement: minimum-bit data representation

Change requirement one, adopted at Tribunal 1 (Week 8). Implemented in firmware before the final demonstration; the Week 13 submission carries its evidence.

**Requirement.** Each application data field uses only the bits its valid range needs, via C bit-fields or explicit masks and shifts (e.g. range 1-5: 3 bits; up to 8 states: 3 bits). Fault conditions (motor stall, encoder fault, IMU fault, no echo, Wi-Fi loss) are individual bits of a shared fault bitmask, not separate integers. Scope: motion/encoder; line/barcode; obstacle; hump estimation; vehicle and controller states; task-to-task and ISR-to-task data; inter-task queue messages; MQTT telemetry and command payloads (compact binary fields, not text values). Read-modify-write of packed data shared between tasks, cores or an ISR and a task must be race-free (atomic access or synchronisation).

**Briefing clarifications.** Loop counters and temporaries are exempt. Small fields may be combined into one integer or char, but do not combine merely because widths match; each owner justifies every width and provides accessor functions that read and write packed fields. Screen sharing in the discussion recording is for illustrating points; reading AI output aloud is not required.

**Week 13 evidence (in the repository):**

1. Data representation table ([team section 6](team.md#6-data-representation)): field, valid range, bits, representation.
2. RAM usage of the final implementation.
3. Message size of the final implementation (inter-task and/or communication messages).
4. Evidence of no lost functionality and no unacceptable timing or performance degradation.

Comparison of the revised implementation with the original is not required (course update, 2026-10-27). Evidence covers the final implementation only; items 2 to 4 are in [team section 7](team.md#7-correctness-and-performance-evidence).

### Historical baseline, issue #22 (not required)

Retained as a record of the pre-change build; the Week 13 evidence does not compare against it.

Compile and link results only; no firmware was executed or flashed. Measured from pinned commits: `main` `c6c4ef9`, `uat` `d8220bc`, both with the same `build_make` makefiles (taken from `main` `ea9c164`). Toolchain: arm-none-eabi-gcc 15.3.1, Pico SDK 2.3.1, macOS host. Command in each `build_make/`: `make clean`, then `make -k -j8 SMP=1 CONSOLE=usb_cdc WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=0`, then `arm-none-eabi-size mtk3pico_smp1_usb_cdc.elf`.

| Build | Warnings | Undefined refs | text | data | bss |
| --- | ---: | ---: | ---: | ---: | ---: |
| `main` | 11 | 0 | 60564 | 560 | 25208 |
| `uat` | 11 | 0 | 86196 | 560 | 26540 |

The 11 warnings are in the pinned sources and are part of the baseline.

| Struct (bytes, via `sizeof` probe, Cortex-M0+) | `main` | `uat` |
| --- | ---: | ---: |
| `S1Telemetry` | 148 | 868 |
| `barcode_service_t` | 488 | 860 |
| `TerrainSnapshot` | 40 | 336 |

`ObstacleSnapshot`, `MotionFeedback` and `MotionCommand` were not found in `main` and are not reported. Task stack is 2048 bytes each (`app_program/app.h`): 4 application task creation sites in `main` (Motion, terrain_task, IR line, Line follow), 7 in `uat` (Motion, Obstacle, Terrain, IR line, Safety, Mission, Reporting); live task counts were not recorded. Not measured: periodic task loop timing, MQTT/telemetry payload sizes (both need an on-target run), per-object `nm` sizes.

## 5. Data representation table (Week 13)

The data representation table, with every field's valid range, bits and
representation, and the evidence that the packed representation keeps
function and timing, is owned by [team section 6](team.md#6-data-representation)
and [section 7](team.md#7-correctness-and-performance-evidence). Earlier
proposals and audits on this page are in git history.

## 6. Assessment, demonstration and process rules

- Assessed: mission completion (whole course, hump height, barcode instructions followed, correct movement), navigation accuracy (a U-turn command executes at the junction, not on reading), completion time, barcode reliability, hump measurement, obstacle avoidance, robustness (a run that works once must work every run),  software design/code quality, resource efficiency, integration, demonstration. Grading is holistic; no weights, penalty amounts or formula were supplied.
- Final demonstration: up to three attempts; the most recent counts, even if worse; the team may stop after an earlier attempt. A good run takes about five minutes; excessive time may be penalised (no hard cutoff or speed mandated). Each attempt is scored on features working (e.g. 2 of 5). No recompilation after the barcode mapping is announced; use the instructor's broker.
- Week 10: partial-integration demonstration (format pending, D10).
- Requirement changes: the Week 8 change is implemented; the Week 13 change is discussed in writing and need not be implemented. AI is used to identify challenges and pivot when requirements change.
- Five members, each owning one subsystem. AI-generated code is permitted, but each owner must explain any line of their submitted code on request; failure attracts a penalty (no amount given).
- Develop drivers and subsystem behaviour, then integrate progressively in pairs or small groups; test for repeatability. A basic micro T-Kernel repository is shared; the instructor teaches no micro T-Kernel but gives a FreeRTOS lab, and learning the kernel is part of the project; ask for help via forums or face to face.
- Group grade is holistic; peer evaluation (affects only the group-work portion of the grade) and contribution evidence (GitHub commits and merges) can raise or lower individual grades. Commit and push regularly with meaningful messages; report serious contribution problems early (anonymous reporting may be possible).

## 7. Course geometry

| Feature | Supplied information |
| --- | --- |
| Black line | 18 mm wide |
| Adjacent barcode lane | 20 mm white gap from nearest line edge |
| Illustrated barcode | about 141 mm long in the photographed travel direction |

**Barcode samples.** Printed samples **A and Z**, each at two scales (larger ~2x). Both have 15 black bars in three groups of five, so bar counts cannot distinguish them. Sequences alternate black/white, starting and ending black; `n` narrow, `w` wide; inter-character gaps narrow. Observed wide:narrow ~3:1 (not a calibrated tolerance).

| Sample | First group | Middle group | Last group |
| --- | --- | --- | --- |
| A | `nwnnwnwnn` | `wnnnnwnnw` | `nwnnwnwnn` |
| Z | `nwnnwnwnn` | `nwwnwnnnn` | `nwnnwnwnn` |

Consistent with Code 39 `*A*` / `*Z*` (inference, not a confirmed spec). Width tolerances, quiet zone, mounting coordinate and junction offset remain pending (D01/D02 in [pending decisions](#pending-decisions)).

Verify print scale and mounting before using any length as a motion distance.

## 8. Not specified by any source

Feature counts/order or full route; multiple barcodes before a junction; issued barcode framing/print tolerances; obstacle depth, bypass/reverse boundaries and numerical accuracy thresholds; grading weights; instructor broker settings. Endless operation is now clarified. Width range and project MQTT schema are defined, but neither supplies measurement accuracy limits.

## Pending decisions

Single register. Record answers and measured values here once, then link from other guides. Nicholas is the decision owner for every row; subsystem labels elsewhere identify the affected code only.

| ID | Decision / missing evidence | Owner |
| --- | --- | --- |
| D01 | Verify issued Code 39 framing/print scales, quiet zones, polarity, width tolerances and encounter rearm; both orientations must yield the same letter | Nicholas |
| D02 | Junction actions reported working on the car (Nicholas, 2026-11-05), so their measurements are no longer tracked. Mounted junction confirmation/turn centre, barcode-to-junction offsets, multiple pending characters, no-command crossing qualification, U-turn pause 200 ms in code, to be measured, bypass/reverse boundaries and failed-scan retry policy; endless/no undo is settled | Nicholas |
| D03 | Verify motor and encoder polarity, counts/revolution, ratings/levels, geometry, full car/turning envelope and maximum edge rate. Remote signs +1/+1 versus local -1/+1 are not a calibration decision; preserve barcode half-range bound | Nicholas |
| D04 | Mounted IR position/spacing/height, contrast/noise/lighting range and barcode black polarity; wiring user-confirmed, measured sensing margins still pending | Nicholas |
| D05 | Sonar electrical behavior user-confirmed; retain recorded Echo-voltage evidence when available. Calibrate repeat interval/range/servo centre/endpoints/settling, sample groups/median/edge hysteresis, coverage and width-estimation uncertainty | Nicholas |
| D06 | Stationary boot IMU calibration (IR is digital, without software calibration): bounded success/failure handshake, axes/bias, impact/filter limits and reference-validated height method/error for humps <=30 mm. Validate 100 mm/s course-speed completion and the accepted results-retained-through-starts/stops-until-reboot lifecycle | Nicholas |
| D07 | Demo stall guard off, full-course guard on (confirmed above); wheel/line gains, speed/duty/distance/turn/stall limits; repeated stationary +/-70 degree sweeps, pivot speed, wheel excursion budget and usable-line handoff; bypass/depth/clearance budgets ; course and bypass driving share `COURSE_SPEED_MMPS`, 100 mm/s today, smoothness first | Nicholas |
| D08 | Stopping distance need not be recorded (Nicholas, 2026-11-05). Output-brake latency versus physical stop distance/time, measured stationarity and U-turn pause start; GP21 release-reset eligibility must not imply physical stationarity | Nicholas |
| D09 | Closed by Nicholas, 2026-11-04: the instructor's WiFi and broker, changing only `config/wifi_secrets.h`. No longer tracked as pending work (Nicholas, 2026-11-05). A normal mission requires both before starting; loss mid-run is report-only. Bounded TX/RX rings stay; the professor's buffer wording was not seen (Nicholas, 2026-11-05) | Nicholas |
| D10 | Qualifying lap agreed above; minimum hump height is 10 mm, not an error tolerance. Lateral (10 mm) and turn (5 degrees) limits agreed above. Agree remaining height/width/speed error and recovery travel limits before corresponding physical PASS; label proposals separately from course thresholds | Nicholas |
| D11 | Settled 2026-10-29: barcode left, hump right. Validate sensing after reverse/U-turn traversal without inventing mirrored lane placement | Nicholas |
| D12 | Closed by Nicholas, 2026-11-03: obstacles sit on straights, and this is the operating assumption. Curved-course bypass is not supported or claimed; options are in the [Obstacle guide](../app_program/obstacle/README.md#curves-near-an-obstacle) | Nicholas |
| D13 | Numeric target priorities/periods, execution budgets, timeout slack, data-age limits, worst sensor-to-PWM latency, GPIO ISR time, stack margin and load-test criteria; user authorized existing numeric settings as provisional software defaults on 2026-10-29; physical scheduling/age acceptance remains open | Nicholas |
| D14 | Core swap superseded. Audit core-0 I/O integration, cross-core event producers/mapping hand-off, synchronization and required initialization order; kernel WiFi/USB and capture core unchanged | Nicholas |
| D15 | Closed by Nicholas, 2026-11-05: the reset watchdog is removed from the firmware, replacing the earlier decision to run it on `full_course`. It never had a bench test. A hung Motion or kernel has no automatic reset. Tribunal A12 names watchdog recovery, and that wording is unchanged | Nicholas |
| D16 | Settled 2026-10-29: configurable 15 cm is the obstacle trigger distance (150 mm). The car stops at that distance: it starts slowing earlier by `motion_stop_distance_mm`, 191 mm at 100 mm/s (Nicholas, 2026-11-04). It does not authorize any travel without valid forward sonar | Nicholas |

Implementation note (2026-10-29, #130): Mission's local reset handshake has
three exclusive states (boot, await GP21 press, await GP21 release), encoded as
0..2 in a private `uint8_t` (2 logical bits, 8 stored; no wire field). Unknown
state cannot request reset. Stop request/service counters remain wrapping
32-bit core-1 counters; reset requires equality inside the output exclusion
section. The software brake acknowledgement is not physical stationarity.
User authorized retaining existing timing/age constants as provisional defaults
for software implementation; D13 physical measurement/acceptance remains open.

Software restructure storage contract (2026-10-29, #131–#133; no new wire fields): historical; the current widths are in [team section 6](team.md#6-data-representation).

| Field / owner | Range and representation | Validity / overflow | Logical / stored bits |
| --- | --- | --- | --- |
| Mission state / navigation phase, Mission | State codes 0 READY, 1 FOLLOW_LINE, 2 NAVIGATE, 3 SCAN, 4 BYPASS, 5 REACQUIRE, 6 STOPPED; phases 0..5 (pause wait, pause, turn, straight, brake, advance) | State 7 invalid -> stop; phases private, produced only by coordinator | 3 + 3 in private byte; published state remains atomic byte, existing 4-bit wire slot |
| Pending mapping, I/O | Existing four 5-bit letters + mapped bit; sequence 0..255 at bits 21..28 | Submit rejects invalid/duplicate letters and occupied hand-off | 29 / one 32-bit word |
| Mapping reply, Mission | Sequence bits 0..7; ACK result 0..3 at bits 8..9 | Reply only after Mission's install/reject decision | 10 / one 32-bit word |
| Mapping progress, one writer per counter | Request/reply tickets plus private submit/consume/take counters, modulo 2^32 | One outstanding request through reply consumption; no overwrite | 32 each |
| Terrain evidence, I/O | Existing terrain word, acquisition/service ms and attempt sequence uint32; acquisition validity 1 bit | Pending data does not renew acquisition time; copy/age/validity required; timestamps wrap | 4 x 32 + 1 logical; naturally aligned 20-byte snapshot |
| IMU startup outcome, I/O | 0 pending, 1 calibrated, 2 failed | Missing IMU publishes failed; never interpreted as calibrated | 2 / one 32-bit word |
| Obstacle step phase/flags, Mission | Phase 0..2 + initialized/ready/scanning; scan counts 0..15, scan ID 0..15, commanded and target angles -60..60 integer degrees | Fixed nine-ray capacity; deadline/cancel/movement abort; no echo invalid; angle targets outside range rejected, no rounding | 2+3 bits; counts 4 each, ID 4, two signed 6-bit angle fields |
| Link readiness, kernel WiFi | bit 0 WiFi, bit 1 broker; boot 0 | Both required for local start; run loss report-only | 2 / one 32-bit word |
| Event drops, event-mutex holder / one writer per core | `comms_event_log_t.dropped` counts ring overflow; `lock_misses[2]` counts failed bounded event-insertion locks (Mission core 1, Terrain core 0), one atomic uint32 per core; bridge-full returns false for retry | Each count saturates at UINT32_MAX; insertion waits at most the existing 2 ms snapshot-lock timeout; counters are debugger diagnostics, not new MQTT fields or delivery acknowledgements | 32 each; 64 stored for the two lock-miss words |

Mission private history conversion contract (historical; current widths in [team section 6](team.md#6-data-representation); 2026-10-30; software precision,
not a physical tuning or accuracy claim; one Mission writer):

| Field | Range / scale / rounding | Invalid / overflow | Logical / stored bits |
| --- | --- | --- | --- |
| Steering target | -127..127, signed Q7 (`line_steer_q`) | -128 unused; fixed correction values, checked by `_Static_assert` | 8 / 8 |
| Last and previous touch | -1 left, 0 none, +1 right (`line_side`, `line_prev`) | only these codes are written | 2 each / 8 each |
| Steering integral | 0..`LINE_I_MAX_Q`, Q7 magnitude, held at the last touch transition (`line_i_q`); side from `line_side`; nearest integer | clamped to the limit, reset 0 | 6 / 8 |
| Junction turn | 0 none, 1 left (+90 degrees), 2 right (-90), 3 U-turn (+180) | none yields zero target (Motion rejects a zero turn); only coordinator sets codes | 2 / 2 in private byte |

Steering keeps no filter history: it stores the Q7 target and integral above and
wheel counts where touches began. The integral between transitions comes from
those counts, so no rounding accumulates. Float remains only in local geometry computation.
Current working-tree target ABI, static RAM and inter-task copy sizes are
recorded in [SYS-12](../testing/system/TEST_LOG.md#results).
The accepted-commit minimum-bit/resource audit remains Stage 9 work.
MQTT byte counts stay unchanged;
the seven-state meaning replaces the old CALIBRATE/COMPLETE state labels, so
clients must use the updated state table. Historical golden bytes remain intact.

Proposed remaining-feature software storage (implementation work; no calibration
or physical acceptance implied): bounded sonar groups store up to five unsigned
16-bit millimetre values; four counters in 0..5 (attempts, accepted, requested,
minimum valid) use three bits each in a private 16-bit unit. The existing Echo invalid code 8191, larger inputs and zero are rejected;
valid values are 1..8190 (13 logical bits per 16-bit array entry).
The valid minimum must be a strict majority. A completed group with too few
valid samples returns unknown, never clear space.
Sorting uses a five-element local copy; even valid counts use the rounded mean
of the two central measurements with 32-bit addition. This defines computation,
not an accuracy rating or an accepted production sample count.

Proposed profile-edge helper contract: configuration is supplied explicitly by
its caller, with no calibration inferred. Depth enter/exit hysteresis, optional
empty-scene background separation, range uncertainty, expected maximum obstacle
width, required adjacent rays and maximum angular sample gap are unsigned
integers in millimetres/degrees as named. UINT16_MAX range uncertainty means
unknown; in that case the width estimate remains an estimate but planning
clearances are zero. Evidence stores unsigned 16-bit angular/range model width
uncertainty and planning width, plus four-bit foreground/free-ray counts and
one-bit edge-valid flags. The model bound excludes unmeasured beam, mounting and
object-shape effects. These remain D03/D05/D10 physical measurements, not a
module accuracy rating. No MQTT fields are added.

Initial steering, 2026-10-29, kept recovery/filter operating choices and a
proposed Terrain new-run reset pending. The later decisions and trial revisions
below supersede that initial proposal: configured values remain provisional,
physical bypass release remains inhibited, and Terrain results persist through
accepted starts/stops until reboot. Unknown/invalid settings stop unsupported
manoeuvres; configured proposals are not physical calibration.

Junction qualifier correction: Mission records the 32-bit timestamp of the last
qualification sample and two one-bit flags (black encounter still open, qualified)
beside its one-bit broad-encounter flag in one private byte. Only distinct usable
black acquisitions may qualify the existing encoder-distance threshold; travel
after black has ended cannot qualify an earlier thin patch.
No mounted geometric tolerance is claimed by these software flags.

Continuation storage: `obstacle_range_sample_t` contains the existing packed word plus
full 32-bit acquisition time, sequence and two wheel counts at
range completion (20 bytes). Counts and consumer-owned origins use unsigned
32-bit storage, masked to `ENC_COUNT_BITS` (currently 16; supported up to 32).
Travel is the mean of each wheel's signed modular displacement from its own
origin, computed locally with half-count precision. Each displacement must stay
below its encoder half range; wheel lifetime counts need not remain close.
Mission retains two junction origins; bypass retains two front-reading and
reacquisition origins. Terrain keeps separate previous-poll counts for stationary
checks and two integration origins, retained across pending samples. Fresh
samples consume integration travel; errors, stale data or invalid Motion
evidence discard the window and reset those origins. No timestamp is renewed
by pending acquisition. Mission owns its current
front/side pair; a separate pair is copied under the existing profile mutex for
optional core-0 reporting, preserving original times when publication is busy.
`obstacle_profile_snapshot_t` combines the 14-byte profile, six-byte edge/model evidence
and a 32-bit scan-completion timestamp (24 bytes). No new RTOS objects or wire
fields are introduced. The 16-bit Echo display timestamp is never a control
freshness clock. The compile-time bypass-release flag is zero while measured
envelope/coverage and the deferred policies are unavailable; this is an inhibit,
not a calibrated threshold.

Decisions confirmed 2026-10-29: recovery always starts left, with targets +70,
-70, +70, -70 degrees relative to the loss heading; Terrain results persist
across accepted starts and stops until reboot. Numerical proposals may now be
implemented, with physical release inhibited. This supersedes the earlier
"keep choices pending" instruction for preparing proposals, not calibration.

User steering, 2026-10-30: normal line tracking has priority. Standalone
recovery starts only after both sensors continuously report white/white
(`IR_LINE_LOST`) for more than 1000 ms of fresh acquisitions. Until then,
hold the last tracking steering and its tracking speed; a visible line resumes
PID updates immediately. Any non-LOST classification resets the loss interval.
Invalid or stale sensing still stops movement rather than extending the grace
period. A sampling gap beyond the existing IR freshness budget cannot count
toward continuous loss.

Loss qualification storage: `line_lost_since_ms` and `line_lost_sampled_ms`
are unsigned 32-bit acquisition timestamps, in milliseconds modulo 2^32,
with wrap-safe elapsed subtraction and no rounding. `line_loss_pending` is
one logical bit packed beside Mission's existing encounter flags; zero makes
both timestamps inactive (all timestamp values, including zero, are valid).
Each state transition clears the pending flag. No shared or wire fields change.

Uncalibrated software proposals for review (not course limits/module ratings):

| Setting | Proposal / purpose | Physical acceptance |
| --- | --- | --- |
| Recovery | After white/white `LINE_LOST_MM` past the last touch (revision 2026-11-02 below), brake then repeat +70/-70 degrees; 36 mm/s per wheel after rolling-scale correction; 30 s / 1200 mm mean absolute wheel excursion bound; 5 degree endpoint tolerance, separate 20 degree overshoot allowance | User requested qualified simplified sweep; D03/D07/D08/D13 pending |
| Recovery handoff | Brake on usable VISIBLE from either sensor; resume tracking without centering or confirmation streak | D04/D07/D10 physical acceptance pending |
| Bypass line alignment | 5 mm/s after rolling-scale correction, independently configured from standalone recovery | D04/D07/D10 pending |
| Scan | Three samples, at least two valid; depth enter/exit bands 30/40 mm; two adjacent rays; model range uncertainty 10 mm | D05/D10 pending; uncertainty is a proposed model input, not sensor accuracy |
| Servo sampling | User-stated +/-60 degree travel (2026-11-03; +/-50 from 2026-11-02 replaced +/-20, which could not see past a 100-200 mm obstacle at the stop distance); coarse rays every 10 degrees and 5 degree edge refinement; proposed 1 degree per 10 ms, 100 ms settle, 60 ms repeat | Verify endpoints/mounting before release, D05 |
| Bypass | `COURSE_SPEED_MMPS` continuous legs of <=550 mm, 36 mm/s pivots, 90 s / 4000 mm whole budget; <=2 s for the side look to settle | D02/D03/D05/D07/D08/D13 pending |
| Bypass geometry | User-measured outline from the axle centre (2026-11-03): nose 155 mm, barcode arm corner 183 mm forward and 85 mm left, right side 75 mm, rear 95 mm, sonar 105 mm forward and assumed on the centreline; 30 mm clearance; 20 mm gap past the box rear (user-chosen 2026-11-05; at least 200 mm of box passed, provisional), 60 degree side look (the servo's measured limit), 90 degree pivots with a leg of box edge plus half the car's length (125 mm, Nicholas 2026-11-05) and at least the 10 mm too-close limit plus a provisional 40 mm allowance from the car's side, offset <=550 mm; conservative obstacle width >=200 mm | Tape measurements; clearance values remain proposals, swept envelope unconfirmed, D03 |
| Rear-edge / return | Two distinct qualified side groups; 150/100 mm enter/release jump; <=1500 mm pass, <=250 mm baseline search; return within 25 mm lateral / 5 degrees and three distinct tracking acquisitions | D02/D05/D07/D10 pending |
| Height quality | Raw acceleration norm 900..1100 mg, wheel speed difference <=30 mm/s, speed change <=30 mm/s per fresh sample, 200 ms steady data before a height window | D06/D07/D10 pending; suppress unqualified height results |
| Retry / reverse | One stationary rescan; two sweeps at each position; blind reverse along the approach, 150 mm in total for one obstacle (user decision 2026-11-03), with no rear coverage | Provisional, D07 |

Recovery trial revision, 2026-10-30: 270 mm/s with 76/127 steering requests
approximately 108/432 mm/s, comparable to the current tracker's maximum
steering at its reduced curve speed. This comparison is not a measured minimum
moving speed. The larger arc radius requires approximately 930 mm for four
nominal target legs; the proposed total travel budget is 1200 mm to leave
alignment margin. Nominal targets stay +/-70 degrees; a discrete sample up to
the existing 5-degree tolerance beyond a target must still allow reversal.
Heading beyond target plus tolerance fails the search. These revised settings
need physical acceptance. Nicholas subsequently requested disabling the motor-stall
guard for the current trial (`MOTION_STALL_GUARD_ENABLED=0`), independently
of `APP_NO_GUARDS`. Command freshness, local stop, action deadlines and
recovery limits remain active. Host tests cover both disabled and enabled
stall configurations; this is not physical acceptance.
On 2026-11-03 Nicholas turned the guard on for the full-course image (`FULL_COURSE=1`); TEST images keep it off. It has no recorded car run.

Storage contract before implementation: recovery keeps two 32-bit encoder origins,
32-bit start/acquisition times and one byte of phase/leg/confirmation flags.
Bypass keeps bounded encoder odometry in signed Q8 millimetres (21 logical bits
per axis within +/-4000 mm), signed centidegree heading, 32-bit timestamps/counts,
16-bit bounded leg/range results and encoded phase/flags. These are private
Mission state; no new task, route planner, event bus or network command is added.
The first qualifying rear-edge position is retained in signed Q8 millimetres
(21 logical bits, +/-4000 mm, copied without rounding from forward odometry).
It is valid only while the edge confirmation streak is nonzero; a new streak
replaces it. Pose overflow fails the bypass before capture. Confirmed rear
position adds the sonar forward mount and baseline ray's forward projection to
this first position, keeping depth and return clearance in the profile's frame.
At the +/-60 degree side look, angle needs eight signed bits;
ray validity needs one more bit. Existing eight-bit angle storage is retained
to preserve the current layouts; commands and constructors enforce the sweep. `ObstacleRay` becomes four packed bytes; `ObstacleProfile`
remains 14 bytes because its previous alignment byte holds validity. Side-watch
uses eight angle bits plus one enable bit in its existing atomic word. Terrain
adds one window-quality bit beside its existing classification/result flags;
completed count/maximum/last results are never reset on accepted starts.


User recovery revision, 2026-10-30: REACQUIRE must stop forward travel and
sweep wider because the moving arc is not finding/tracking the line. The
current software proposal supersedes the earlier moving +/-70 degree trial:
stop, confirm stationarity, then pivot to +100/-100/+100/-100 degrees from
the stopped pose. Brake and confirm stationarity before reversing. A visible
line brakes the sweep immediately, then bounded 10-degree corrections center
it. Confirmation uses three distinct stationary acquisitions, without forcing
the robot back toward its loss heading. This only applies to standalone loss;
bypass retains its original-line pose checks. The 180 mm/s wheel speed and
100-degree extent are uncalibrated proposals, not measured limits. The 30 s
budget includes braking; the 1200 mm bound now measures mean absolute wheel
excursion from the stopped pose, not cumulative travel. Four legs and the
timeout bound oscillation. Freshness, stop, front hazard and action failure
still abort recovery.

User recovery handoff revision, 2026-10-31: supersedes the stationary
centering step above for standalone recovery. Brake when either sensor sees
a usable VISIBLE line, then confirm three distinct stationary VISIBLE
acquisitions and return to line tracking regardless of left/right error.
Ordinary tracking corrects the remaining offset; recovery no longer issues
10-degree alignment pivots. Losing visibility clears confirmation and resumes
the bounded sweep. Broad black alone does not confirm a trackable line.
The >1 s white/white qualification and 150 ms encoder-stationarity check
remain unchanged software settings, not measured timing results. Bypass
original-line checks and recovery failure guards remain unchanged.

User sweep revision, 2026-10-31: the reported short, slow single sweep calls
for wider and faster back-and-forth search. Trial targets are now
+120/-120/+120/-120 degrees, with 240 mm/s maximum per pivot wheel.
Keep braking and the 150 ms encoder-stationarity check at each reversal.
Separate the 5-degree endpoint tolerance from a 20-degree overshoot allowance:
coasting beyond an endpoint must not prematurely exhaust recovery, while
absolute heading beyond 140 degrees still fails. The existing 30 s, four-leg
and 1200 mm excursion bounds remain. These are uncalibrated software proposals;
actual angles, wheel speed and stopping excursion remain pending D03/D07/D08.
The stable-visible handoff above is retained. No storage or wire widths change;
the largest reversal from the allowed heading is 260 degrees, within Motion's
signed 360-degree target limit.

User-reported D03 observation, 2026-10-31: in response to the requested
one-full-wheel-revolution manual check with GP21 held, Nicholas supplied encoder
counts 31142 before and 33655 after: delta 2513 decoded counts. Wheel identity,
other-wheel scale and repeatability remain pending; this is not a completed
calibration or independently measured physical acceptance. See
[S2-U07 observation](../testing/motion/logs/2026-10-31_S2-U07/observation.json).
The current 360-count setting underestimates this delta by a factor of 6.981;
under existing diameter/track assumptions, it predicts a physical 10.03 degree
turn for an estimated 70 degree pivot. Applying this scale also changes wheel
speed targets, distances, telemetry and terrain integration, so motor-speed
settings must be reconciled before a powered trial. No code calibration has
been changed from this single-wheel observation.

Follow-up D03 observation, 2026-10-31: Nicholas identified the 2513-count wheel
as the right wheel. For the left wheel, after setup movement invalidated the
first baseline, stable telemetry was 65501 before the requested manual forward
revolution and 2454 after. Signed 16-bit wrap-safe travel is +2489 counts.
Both observations are close to 2500 counts/revolution (difference 0.96% of
their mean); manual angle placement and repeatability remain unmeasured.
See [S2-U08](../testing/motion/TEST_LOG.md#results).
At that observation stage, no encoder/motor setting had changed. A manual 500 mm straight rolling
measurement is next to establish effective counts/mm without powered motion;
awaiting the operator's positioned baseline.

Applied rolling-scale revision, 2026-10-31: S2-U08's operator-reported 500 mm
manual straight roll produced left +5833 and right +5816 counts. Effective
scales are 11.666 and 11.632 counts/mm; use their mean 11.649 counts/mm as the
single odometry constant. Their relative difference is 0.292%; repeatability,
measurement uncertainty, loaded slip and track width remain D03/D07. This
direct rolling calibration replaces the provisional revolution/diameter
formula; manual-revolution observations remain evidence, not runtime constants.
No physical wheel diameter is inferred from the two different procedures.

Speed reconciliation preserves approximately the previous encoder counts/s:
follow 68, broad 14, turn/clear 15, recovery 36, bypass 9 and bypass alignment
5 mm/s. These are whole-mm/s trial setpoints, not measured achieved speeds;
wheel PID gains and duty cap are unchanged. The bounded action/navigation
deadline becomes 12 s: a 180-degree pivot and post-turn clearance now represent
their actual requested distance rather than an approximately one-seventh arc.
The existing 30 s recovery deadline remains. Longest 2000 mm individual
distance target is 23298 counts, within the signed 16-bit half range; the
1200 mm recovery excursion is 13979 counts. Whole-bypass heading uses modular
right-minus-left displacement so long forward travel cannot alias each
wheel's signed origin subtraction; differential heading stays bounded to
180 degrees. Shared/wire counters remain 16 bits. Live decoder reads the
same direct counts/mm constant. S2-U09 owns software verification; no powered
angle, distance, speed-loop or stopping acceptance is implied.

Simplified standalone recovery revision, 2026-10-31, supersedes the loss delay,
sweep extent/leg count and stationary-visible confirmation above. During
ordinary tracking, the first fresh usable white/white (LOST) acquisition brakes
and cancels held/manual navigation, barcode work, scan and side-watch intent.
Once stopped, pivot between +70 and -70 degrees from the stopped pose until
either sensor produces usable VISIBLE line evidence; brake that pivot and
return to ordinary tracking without a confirmation streak or centering step.
Barcode capture restarts with old events dropped on return. BROAD_BLACK alone
does not qualify as a trackable line. Repeat sweeps until the existing 30 s,
wheel-excursion, heading, obstacle, freshness, stop or actuator-failure bound;
there is no four-leg cutoff. Pivot speed remains the uncalibrated 240 mm/s
trial setting, endpoint tolerance 5 degrees and overshoot allowance 20 degrees.
Deliberate junction turns, the requested 25 mm straight segment and bypass
return are separate policies and retain their existing behavior.

Private storage revision: remove the two loss-qualification timestamps and
pending bit from Mission. Recovery uses four phase values plus invalid (3 bits),
one sweep-direction bit, one active-turn bit and the existing signed 10-bit
relative target. Keep two 32-bit encoder origins and the 32-bit start time;
remove confirmation count and stored acquisition time. No shared/wire change.

Recovery takeover delay revision, 2026-10-31: supersedes immediate takeover
above. Ordinary tracking retains its last steering and corresponding speed
while fresh usable white/white persists for less than 500 ms. At >=500 ms,
brake, cancel pending navigation/barcode/scan/side-watch work and enter the
existing simplified +/-70 degree sweep. VISIBLE or BROAD_BLACK clears the
pending loss; stale/invalid sensing and stop/hazard still fail immediately.
Use original acquisition times, never loop wall time; a gap beyond the IR
freshness budget restarts qualification rather than counting missing evidence.
The 25 mm straight segment remains independent; if LOST at completion, ordinary
tracking starts this qualification then. The sweep and immediate usable-line
handoff remain unchanged. With the corrected nominal 68 mm/s setting, 500 ms
represents 34 mm of commanded travel; achieved travel and safe trial limits
remain unmeasured D07.

Private loss-qualification storage is restored: unsigned 32-bit first/latest
LOST acquisition timestamps in milliseconds modulo 2^32, wrap-safe subtraction,
and one pending bit beside existing Mission flags. Timestamp zero is valid;
the bit denotes activity. State changes clear it. No shared/wire fields change.

Line-loss revision, 2026-11-02 (INT-127; host and build only): supersedes the
500 ms qualification above. DOUT white/white is also the centred reading, so time
alone stopped good straights, and the tracker never produced `IR_LINE_LOST`.
Loss is now white/white for `LINE_LOST_MM` (350 mm, trial value, INT-135) of encoder travel
past the last one-black or broad-black acquisition, counted only after the first
touch since entering FOLLOW (before it the line is between the sensors and cannot
leave unseen). The steering drift toward the last side makes a centred robot touch
again within about 50 mm at the trial drift. The timestamps and pending bit are
removed. Recovery and the bypass line return accept white/white alignment only
after a sensor has been black during that phase; a one-black sample still ends
recovery as before.

Historical recovery storage revision before implementation: phase has five values plus
invalid (3 logical bits; unused codes fail), leg 0..4 (3 bits), confirmation
0..3 (3 bits, configurable through 7), and active-turn flag (1 bit). Held
relative turn target is signed whole degrees -360..360 (10-bit signed bit-field,
-512 reserved invalid, nearest rounding with ties away from zero; heading
bounds and Motion construction reject out-of-range values). Two encoder
origins and two unsigned 32-bit timestamps retain their existing wrap rules.
All storage is private to Mission; no shared or wire layout changes.

Digital IR calibration removal (2026-10-31): `IrCalibration`, capture enums,
capture counters/results and all IR calibration gates/APIs are removed.
`IrTracker` stores only line, reading, raw word and acquisition presence/time;
`LineIrSnapshot` stores acquisition identity/age, raw word, digital endpoints/error,
line and freshness/validity flags. Existing field widths/scales are retained;
the older aggregate struct-size rows describe their recorded revision, not a
current target size measurement. IMU calibration remains separate.

Junction acceleration revision (2026-10-31): no new stored fields or wire layout.
Mission reuses its two unsigned encoder origins (`broad_origin`) for each active
navigation phase, reset at turn start and clear start. Local progress is 0..1,
clamped from signed travel/heading; wrong-direction movement cannot increase
turn progress. Straight target remains 25 mm; turns use commanded angle and
track width to calculate pivot arc, combined with existing 60 mm clearance.
Speed requests are integer mm/s rounded down, half-to-full current tracking
speed (35..70); existing Motion braking/tolerances/timeouts still apply.
Tracking correction is +/-127 Q7, yielding 0/140 mm/s wheel targets at 70.
Obstacle bypass keeps its prior independent 7 mm/s pivot speed.

Tracking smoothing revision (2026-10-31, INT-80; provisional D07): Mission's
`line_steer_q` retains signed Q7 -127..127 (8 logical/stored bits, -128 unused).
It moves at most 64 Q7 units per distinct usable acquisition toward the digital
target, saturating at that target; state entry resets it to zero. New private
`line_steer_sampled_ms` is the last processed acquisition time, unsigned 32-bit
wrapping milliseconds (32 logical/stored bits; all values valid; equality
rejects repeated acquisitions). Mission is the sole writer; no wire change.
Normal line tracking requests 45 mm/s and retains a 70% single-wheel boost;
full correction requests 45/76.5 or 76.5/45 mm/s before wheel PID. Navigation
retains its separate half-to-full 70 mm/s ceiling. These values are proposed
physical tuning, not calibrated limits or an observed tracking pass.

Curve authority/speed revision (2026-10-31, INT-81; provisional D07): supersedes
INT-80 tuning values. Normal tracking requests 48 whole mm/s; navigation
ceiling is 75 mm/s (half-speed requests round down). Existing Q7 steering
ramp and acquisition history are unchanged; no stored/shared fields added.
At |steering| <=64 the inner wheel retains base speed; from 64 to 127 its
target decreases linearly to 60% of base. Outer boost remains linear to 170%
of base. Full tracking correction requests 28.8/81.6 or 81.6/28.8 mm/s; these
are local float wheel targets, not new packed command fields. Zero remains
base/base; existing duty cap, stop and expiry apply. No physical pass inferred.

Curve-speed ramp revision (2026-10-31, INT-82; provisional D07): the working
tracking ceiling is now user-selected 75 mm/s. Mission adds private
`curve_speed_step` (unsigned 0..10, 4 logical/stored bit-field bits; 11..15
invalid, speed accessor returns zero) and `curve_direction` (0 none, 1 negative
Q7, 2 positive Q7; 2 logical/stored bits, 3 unused). Mission alone writes
both, packed together; no wire/API change. State entry starts at full-speed
step 10, direction zero. A new nonzero digital correction direction starts
at step zero (50%); each subsequent distinct usable acquisition advances
one step, saturating at 10 (100%). At 20 ms acquisition cadence the ramp
takes 200 ms. A direction reversal restarts at 50%; repeated copies and LOST
holds do not advance it. White guidance continues acceleration, clearing
direction only once steering has settled to zero. Broad black clears the
curve history. Existing speed scaling for a hump and integer downward
rounding apply after this ramp; both wheel targets scale together. Tracking
requests 37..75 mm/s at this setting; actual wheel speed/acceleration and
duty saturation are not established by the command ramp. Existing navigation
uses its separate encoder-progress ramp. No physical acceptance inferred.

INT-82 working tuning identity: preserve the concurrent user-selected 0.85
outer-wheel boost (185% at full Q7), with the existing 40% inner reduction.
At the selected 75 mm/s full curve speed, local targets are 45/138.75 or
138.75/45 mm/s. The duty cap can constrain achieved speed; ramp percentages
refer to commands, not physically measured acceleration.

Smooth FOLLOW revision (2026-10-31, INT-83; provisional D07): supersedes
INT-82's acquisition-count speed reset. Remove Mission's `curve_speed_step`
and `curve_direction`; tracking requests its configured ceiling, with existing
hump scaling. Motion owns two private unsigned Q6 mm/s wheel-target histories,
`follow_speed_q6[2]`: range 0..59200 (0..925 mm/s at current maximum command
and boost), nearest rounding, 16 bits each including invalid 65535. Zero is
stationary demand; values above the configured maximum are invalid and brake.
Arithmetic saturates toward the valid requested target without overshoot.
The provisional slew limit is 250 mm/s per second, both increasing and
decreasing; elapsed progress is capped at one Motion period after a late
release. Startup permits one period of progress, duplicate timestamps none.
FOLLOW zero speed, STOP, invalid/expired commands, reset and faults bypass
the ramp and clear history. Discrete distance/turn endpoint control is unchanged.
Physical acceleration, curve retention and PID acceptance remain unmeasured.

Navigation wheel/IR revision (2026-10-31, INT-84): user requests left turns
with left braked/right forward, right turns with left forward/right braked,
and clockwise U-turns with left forward/right reverse. User subsequently
confirmed turning-side IR completion for left/right as well: left IR for
left, right IR for right and U-turn. TURN reuses its existing steering slot
as a style: 0 pivot (existing recovery/bypass), 1 single-wheel forward swing;
other TURN styles are invalid and brake. No command layout/size or wire change.
Swing driven-wheel arc is track_width * angle; the braked wheel never gets
position correction. Mission sends +/-360-degree bounds (left positive,
right/U negative), which are failure ceilings, not requested completion angles.
MissionInputs adds transient `left_black` and `right_black` from normalized
digital sensors, meaningful only with a fresh usable acquisition; line error
cannot distinguish both-black from both-white. Private Mission adds a 2-bit
`turn_line_phase`: 0 leave starting black, 1 wait for new black, 2 wait for
white, 3 invalid -> stop. The existing 32-bit confirmation timestamp becomes
`turn_sampled_ms` for distinct acquisitions. Remove the obsolete visible
streak and CLEAR/CONFIRM phases. Navigation has five phase codes 0..4 (pause
wait, pause, turn, straight, brake), 5..7 invalid; three bits include invalid.
On navigation entry reset edge history. After the turning-side IR leaves the
starting black, require a fresh new black then white; hold STOP until Motion reports IDLE (serviced software braking), then
enter FOLLOW_LINE and run normal tracking on the next release. Mission runs
at 10 ms while Motion runs at 20 ms, so a one-release STOP can be missed;
this uses existing feedback, adds no stationary gate and is not proof of
physical stationarity. This removes
the 60 mm clearance and outgoing confirmation gates for left/right and U-turn.
The stationary 200 ms U-turn pause and straight encoder segment remain.
Encoder progress ramps the speed ceiling over nominal 90-degree swing or
180-degree clockwise pivot travel, without determining completion. Preserve
command expiry using original IR sample time, the 360-degree failure bound,
existing action timeout, obstacle and stop guards. The first eligible post-exit
crossing is accepted; course geometry and sensor positioning require physical
verification (D02/D03/D07/D08), not a host-derived guarantee of exact heading.

Navigation approach/curve response revision (2026-10-31, INT-85): user
requests 70 mm forward before left/right navigation turns. On the queued
junction acquisition consume the direction once, store wheel origins and
enter new private phase ADVANCE (code 5; 3-bit phase field, 6/7 invalid).
Request straight FOLLOW at normal tracking speed until signed wrap-safe mean
wheel displacement reaches 70 mm using ENC_COUNTS_PER_MM; reverse displacement
and elapsed time do not qualify. Reset turn origins and the IR edge sequence
when advance finishes. Seed edge phase 1 if the turning-side IR is already
white, otherwise 0 to leave black before seeking new black/white. Then use the
existing single-wheel turn and serviced STOP-to-FOLLOW handoff. U-turn keeps
its existing stationary pause and clockwise sensor-ended pivot, without
70 mm advance. The existing overall navigation timeout includes approach.
70 mm is user-selected geometry, not a calibrated sensor/axle offset.

Straight navigation requests normal tracking speed throughout its existing
25 mm segment; remove its half-to-full encoder speed ramp. Turn rotation
retains its encoder-progress speed ceiling. Hump scaling and immediate stop
policies remain. Motion now ramps the common FOLLOW base speed BEFORE wheel
mixing, so inner/outer target ratios respond immediately to steering rather
than both targets climbing equally after a turn. Replace the two unsigned
Q6 wheel histories with one private `follow_base_q6` field: 0..32000 (0..500
mm/s), exact whole-mm/s requests scaled by 64, truncating per-release slew
increments, 15 bits including invalid 32001..32767; reset zero, invalid brakes,
updates saturate toward requested speed without overflow. (Superseded 2026-11-04 by the lift-style profile in [architecture](architecture.md).) The 250 mm/s/s
provisional base ramp, one-period cap on late releases, no progress for
duplicate timestamps and immediate zero/stop/fault reset remain. Mixed wheel
targets are local floats, bounded by existing mixing and duty limits; a wheel
may change faster than the base during steering. No physical curve pass or
calibrated acceleration is inferred.

Steering smoothness tuning (2026-10-31, INT-86; provisional D07): after
reported jerky curve traversal, reduce LINE_STEER_STEP_Q from 64 to 32 per
distinct fresh acquisition. Retain the common-base speed ramp and full
steering/mixing limits, so correction starts on the first sample and sustained
curve authority remains. Zero-to-full and full-to-zero take four 20 ms
acquisitions (80 ms); full reversal takes eight (160 ms). No stored field,
API or wire change. The 70 mm left/right approach, navigation speeds,
IR turn completion and stop/expiry policies remain. These are user-triggered
provisional tuning values; physical curve retention remains unmeasured.

Faster tracking trial (2026-10-31, INT-87; provisional D07): user requests
faster line following. Increase MISSION_TRACK_SPEED_MMPS from 75 to 90 mm/s
(+20%). Normal tracking, straight navigation and the 70 mm L/R approach
share this ceiling; existing hump scaling requests 45 mm/s. Rotation ceiling
remains 75 mm/s, broad-black qualification remains 37 mm/s, and 32-Q7 steering
steps/common-base acceleration remain. Tracking no longer inherits the
rotation/broad speed ceiling; validate it directly against Motion's speed
limit and include tracking's outer-wheel boost in the compile-time bound.
At sustained full correction, mixed tracking targets become 54/166.5 mm/s
or the reverse. Duty saturation, curve retention and stopping distance at
this setting are not measured; this is a requested-speed change, not physical
acceptance. No persistent field, API or wire layout changes.

Gentler curve authority trial (2026-10-31, INT-88; provisional D07): after
reported excessive curve correction/overshoot at the faster setting, reduce
MOTION_FOLLOW_BOOST from 0.85 to 0.60 and MOTION_FOLLOW_INNER_CUT from
0.40 to 0.25. Full correction now requests inner 0.75 * base and outer
1.60 * base: 67.5/144 mm/s at 90 mm/s, or the reverse. Wheel target difference
is 76.5 instead of 112.5 mm/s (32% less). Preserve the Q7=64 tightening
threshold, 32-Q7 acquisition steps, common-base ramp, 90 mm/s tracking,
70 mm approach, rotation speed and IR handoffs. Shared FOLLOW mixing also
applies to bypass; discrete swing/pivot targets are unchanged. No stored
field, API or wire changes. These are provisional tuning changes following
a user report, not calibrated physical curve acceptance.

IR response trial (2026-10-31, INT-89; provisional D07): after user reports
running wide / late turning and jerky straights, retain the gentle first
32-Q7 correction but use 64-Q7 steps while correction is nonzero. Opposite
black first unwinds to zero; a fresh acquisition then enters the new direction.
Entry is 32/96/127, white release 127/63/0, full reversal 127/63/0/-32/-96/-127.
Tracking base speed scales linearly with absolute steering from 90 to 45 mm/s;
hump scales that request by half afterward. At full correction the requested
mixed wheel targets are 33.75/72 mm/s or the reverse, before Motion's common
base ramp and wheel PID. This supersedes INT-86's symmetric steps and INT-83's
fixed tracking ceiling, retaining existing mixing and navigation settings.
No new stored field, interface or wire representation. Physical comparator
thresholds, oscillation, curve retention and loaded speed remain uncalibrated.

Curve entry/inner-wheel trial (2026-10-31, INT-90; provisional D07): user
reports initial jerk and insufficient steady turning, requests slower inner
or faster outer wheel. Reduce LINE_STEER_STEP_Q from 32 to 24: first step
24, subsequent steps 48, giving 24/72/120/127 entry, 127/79/31/0 white
release, and reversal through zero before the opposite gentle entry. Entry
now needs four acquisitions instead of three; confirm the added delay on
actual curves. Increase MOTION_FOLLOW_INNER_CUT from 0.25 to 0.50 above
unchanged Q7=64 tightening threshold. Full inner target is 50% of base;
outer remains 160%. At the 45 mm/s full-curve base request, targets are
22.5/72 mm/s or the reverse, versus INT-89's 33.75/72. The common-base ramp
and wheel PID still determine actual wheel response. Shared FOLLOW mixing
also affects bypass guidance. Straight/approach, rotation, stop, freshness
and hump policies retain their settings. No stored field/API/wire change.
These supersede INT-89 entry timing and INT-88 inner cut; loaded speed,
curve retention and smoothness remain unmeasured D07/T03.

Curve/straight-exit revision (2026-10-31, INT-91; provisional D07): continued
wide-curve and angled straight-junction report. Increase FOLLOW inner cut
from 0.50 to 0.70, retaining Q7=64 tightening onset and 0.60 outer boost.
Full-curve targets at the requested 45 mm/s base are 13.5/72 mm/s or the
reverse; Motion slew/PID response determines actual wheel speed. This shared
mixing also affects bypass guidance. Entry timing remains INT-90.
During the 25 mm straight-navigation segment use fresh visible IR guidance
and normal tracking curve-speed scaling instead of forcing steering to zero.
Broad-black/no-guidance crossing clears steering history; invalid VISIBLE
guidance stops. Preserve steering and acquisition time on the transition
back to FOLLOW_LINE. Encoder completion, local stop, stale data, obstacle
and navigation timeout checks remain; L/R approach remains zero-steering.
No new stored state/API/wire changes. These supersede INT-90's inner cut
and earlier fixed-steering straight-segment descriptions. This can correct
an angled exit while a sensor sees the line; two digital white inputs still
cannot prove centering, loss or heading. Slower inner-wheel stall/overshoot,
curve retention and junction geometry remain physical D07/T03 trials.

FOLLOW steering storage (2026-10-31, INT-92; proposed before implementation):
Motion owns private `follow_steer_q`, signed Q7 -127..127; 8 logical/stored
bits including invalid -128, zero rounding (integral slew steps toward target).
Invalid -128 brakes and resets. Proposed slew is 800 Q7 units/s, elapsed
progress capped at one Motion period, startup one period, duplicates none.
At 20 ms this is 16 units/release, full entry/release eight releases (160 ms),
full reversal sixteen (320 ms). Saturate toward requested steering with no
overshoot; no progress from elapsed time beyond one release. STOP, zero
FOLLOW speed, invalid/stale commands, reset and faults clear steering history.
Mission retains only the latest acquisition's digital target, removing its
second smoothing stage. No wire/API change; Motion alone writes motors.
Full FOLLOW inner reduction becomes 1.0 (inner braked), outer boost stays
0.60. Mission's curve base request becomes floor(tracking ceiling /
(1 + 0.60 * absolute requested steering / 127)), then existing hump scaling:
90 mm/s straight and 56 mm/s at full correction, yielding settled 0/89.6
mm/s targets, or the reverse. During transitions, base and steering slew
independently; constant outer-wheel physical speed is not guaranteed.
This supersedes prior Mission steering steps and half-speed curve scaling.
Response time, comparator chatter, loaded PID, curve retention, stopping
excursion and pivot geometry remain provisional D07/T03.

Encoder PID diagnostic revision (2026-10-31, INT-94): expose best-effort
per-release wheel targets, measured speeds and serviced duties without changing
PID gains, encoder scale, steering or travel settings. Use the
[Motion tuning procedure](../app_program/motion/README.md#wheel-speed-tuning) to separate
motor response from changing steering demands. D07/D13 remain pending for loaded
response, target saturation, timing/stack impact and physical tracking; no newly
calibrated gains or motion pass is claimed. The former dashboard black/white
capture checklist is removed to match digital acquisition.

## DOUT navigation commissioning

User-approved design, 2026-11-01. This supersedes the earlier immediate queued
both-black trigger, 25 mm straight exit, 70 mm advance and IR-only +/-360-degree
navigation policies. It does not establish physical acceptance or new course rules.
Control behavior is owned by [architecture](architecture.md#dout-navigation-contract).

**Measurement status (Nicholas updates, 2026-11-01; not a physical motion pass):**

| Decision | Required measurement / evidence | Current state |
| --- | --- | --- |
| D03 | Effective driven-wheel separation; rolling scale/sign per wheel | Wheel-centre separation user-measured 120 mm; effective turning width and individual rolling verification pending; existing mean scale retained |
| D02/D03 | Chassis centre forward and left of drive axle midpoint | Unknown; zero config is a placeholder, not a measurement |
| D04 | Both tracking sensors forward/left of axle, optical centre spacing, height, polarity and dead zone | User-measured 120 mm from axle line to IR pair centre after moving the pair back 18 mm (`NAV_SENSOR_FORWARD_MM`; was 135, INT-117); modules 30 mm apart, outside both line edges. INT-98 slide: effective spot spacing 33 mm, effective line width 23 mm (right sensor), centred white/white band 10 mm, console polarity black=1. Left-sensor span, hysteresis, lateral offset and height pending |
| D01/D04 | Barcode sensor forward/left of axle, mounted polarity, narrow/wide print widths, quiet/rearm zones | User reports 65 mm ahead of and 40 mm left of the left IR sensor, moved back 18 mm with the IR pair (INT-118), so about 185 mm forward and 55 mm left of the axle midpoint if the IR pair is centred; polarity and quiet/rearm zones unknown; user measures a narrow black bar on the course print at about 5 mm (S3-U35); decoder limits provisional. User constraint (2026-11-02): the barcode sensor stays at the line sensors' height, because on a hump a lower sensor would hit the floor; narrowing its spot must not lower it (S1-U26). User decision: the barcode sensor stays digital, no analog output |
| D02 | Crossbar width, switching-position uncertainty, body swept clearance, tightest curve traces | Line and crossbar both 18 mm (two intersecting lines); a crossing reads both-black for about 23 mm of travel (INT-98 spot width); clearance and tightest-curve traces pending |
| D07/D08 | Loaded speed response, braking distance, stationarity, arc error limits and pause behavior | Physical trials pending |

`NAV_GEOMETRY_RELEASED=0` inhibits newly qualified left/right/U-turn navigation;
straight and unqueued crossings continue with tracking. Enabling asserts that all
measurements, clearance and thresholds have been validated. Sensor forward offset
and crossbar width also must be positive. Stop on infeasible geometry; no fallback
pivot. U-turn currently requires a laterally centred chassis (`y_c=0`); a nonzero
lateral offset cannot be corrected by an in-place 180-degree pivot alone.

Provisional commissioning settings, not measured limits: tracking at 100 mm/s
with IR read every 10 ms. Steering (proposed trial values; 115/127/57 veered off
in INT-132, and the INT-123 PI defaults are in git history): one sensor black
P 45/127 plus I growing 1/127 per mm of that touch up to 45/127; on white/white I
fades 0.5/127 per mm, never below a 8/127 drift toward the last side. Broad black requests
none; at broad entry, black onsets more than 15 mm apart mark curve skew (trial
value, to be set from type 9 line traces). Motion releases to zero at
1800 Q/s, reverses at 600 Q/s until zero and builds at 300 Q/s (about the 250 mm/s2 course limit at each wheel at 100 mm/s; lowered 2026-11-05 for the jolt on straight-line touches); steering is mixed centre-preserving, left
v(1+q/127) and right v(1-q/127), so the robot centre keeps the commanded speed
(the outer wheel reaches 200 mm/s at full steering; INT-129). Wheel feedforward KF 0.0001, KS 0.05, KP 0.0001
(S2-U11 to S2-U13). Candidate travel is 6 mm (INT-189; 9 mm in INT-98 missed real crossings, INT-188). A straight crossing rolls 36 mm at course speed with no steering, without stopping, then tracks (INT-189). Curved turn entry then stops rather than assuming pose.
Entry-relative yaw bound 5 degrees; junction pivots (trial values copied from the
reference repo, no physical run) are bounded at 120 degrees for left/right and 210
for U-turn, look for the new branch from 60 and 150 degrees, at the 100 mm/s
course speed; rearm 10 mm. All require measurement
under D02/D07. Pause is 100 ms after stationarity before and after every turn; nominal approach acceleration uses requested speed / 0.5 s.
Actual approach distance and completion remain encoder governed.

Storage extension (one writer per object; local floating point geometry only):

| Field | Range / scale | Invalid / overflow | Logical bits / storage |
| --- | --- | --- | --- |
| Motion discrete target latch | -2000..2000 mm or degrees, whole units | Command mutation fails action | 12 / signed bit-field |
| Motion style latch | 0 pivot, 1 legacy swing, 2 invalid (removed smooth path), 3 ramped | Constructor validates | 2 / bit-field |
| Mission output navigation rejection | 0 none, 1 measurements, 2 geometry, 3 entry | Console reports on stop transition; no wire change | 2 / bit-field |
| Mission navigation phase | 0..6, 7 invalid | Invalid stops | 3 / existing field |
| Mission turn advance | 1..2000 whole mm | Invalid stops before construction | 12 / signed bit-field |
| Mission locked/curved/exit flags | independent 0/1 | Reset/start manages encounter | 1 each / bit-fields |
| Mission entry origins | encoder counts modulo configured width | Half-range modular arithmetic | 2 x ENC_COUNT_BITS / uint16 |
| Barcode diagnostic | 2 wrapping counts, width 0..65535 counts; result 0..6; orientation 0/1; letter 0..25 or 31; action 0..4 | Width saturates; queue drops counted; no guessed letter | 16+16+16+3+1+5+3 / 8 bytes on target |

MotionCommand remains 12 bytes with the existing mutex and two-bit mode. TURN
style 2 is rejected (the smooth path was removed 2026-11-02); other TURN styles
take degrees. DISTANCE style 3 enables
a ramped approach. No MQTT payload changes. Diagnostic ring is optional, 32 entries;
head/tail and saturating dropped counters are uint32 for wrap-safe lifetime indices.
