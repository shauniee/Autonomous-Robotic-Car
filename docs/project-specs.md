# Project specifications

## Contents

- [Source provenance and requested design](#source-provenance-and-requested-design)
- [1. Mission and required outcomes](#1-mission-and-required-outcomes)
  - [Encounter behaviour](#encounter-behaviour)
  - [Course rules](#course-rules)
  - [Mission-day barcode rule](#mission-day-barcode-rule)
- [2. Subsystem deliverables](#2-subsystem-deliverables)
- [3. Communications](#3-communications)
- [4. Change in requirement: minimum-bit data representation](#4-change-in-requirement-minimum-bit-data-representation)
  - [Historical baseline, issue #22 (not required)](#historical-baseline-issue-22-not-required)
- [5. Data representation table (Week 13)](#5-data-representation-table-week-13)
  - [S1 Communications (proposed; frame layouts in architecture; needs Shaun's sign-off)](#s1-communications-proposed-frame-layouts-in-architecture-needs-shauns-sign-off)
  - [S2 Motion (encoder width provisional until D03; one constant `ENC_COUNT_BITS` in `robot_config.h`; branches #29, #31)](#s2-motion-encoder-width-provisional-until-d03-one-constant-enc_count_bits-in-robot_configh-branches-29-31)
  - [S3 Line, barcode and Mission (branches #35, #36, #37; pending rows converted by #52 after Nicholas confirms)](#s3-line-barcode-and-mission-branches-35-36-37-pending-rows-converted-by-52-after-nicholas-confirms)
  - [S3 bit-usage data contract (proposed; spec #100; final evidence #107)](#s3-bit-usage-data-contract-proposed-spec-100-final-evidence-107)
  - [S4 Terrain (scale provisional, 1 mm per count `HUMP_MM_PER_COUNT`, until D06; branches #45, #46)](#s4-terrain-scale-provisional-1-mm-per-count-hump_mm_per_count-until-d06-branches-45-46)
  - [S5 Obstacle (proposed; branches #48, #49)](#s5-obstacle-proposed-branches-48-49)
  - [Mission and Safety](#mission-and-safety)
  - [Fault bitmask (`app_program/common/robot_data.h`; see architecture for status)](#fault-bitmask-app_programcommonrobot_datah-see-architecture-for-status)
- [6. Assessment, demonstration and process rules](#6-assessment-demonstration-and-process-rules)
- [7. Course geometry](#7-course-geometry)
- [8. Not specified by any source](#8-not-specified-by-any-source)
- [Pending decisions](#pending-decisions)

Single owner of requirements, the change requirement (CR), the Week 13 data table and pending decisions. Sources: the written brief and the instructor briefings ("briefing" below). Discrepancies between sources were resolved with the team on 2026-10-25 and are recorded here. Week 8 realigned plan: [tribunal1](additional-docs/tribunal1.md). Design: [architecture](architecture.md). Members, hardware, workflow and coding rules: [team](team.md). Subsystem guides: [S1](architecture/subsystem1.md), [S2](architecture/subsystem2.md), [S3](architecture/subsystem3.md), [S4](architecture/subsystem4.md), [S5](architecture/subsystem5.md).

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

**Reconciled design, 2026-10-29:** [plan](../plan.md) owns implementation
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
before IR calibration; IMU failure is flagged but permits operation. During a run
IR/Motion failure stops; ordinary IMU/sonar/network loss is report-only. Requested
hump speed is half normal tracking speed; requested moving recovery is left/right
70-degree sweeps for two back-and-forth attempts, with exact travel/sequence still
pending. Local stop retains calibration, mapping and last barcode display, but
cancels pending movement. GP21 release may reset after cause clearance/brake;
GP20 starts separately. No two-second barcode disabling rule remains.

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
| M7 | Profile obstacles with ultrasonic sensing (location, width, closest point, left/right clearance); keep observing while bypassing to measure depth and detect when passed |
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
| Blocking obstacle | Scan left/right with the servo ultrasonic sensor, estimate width and clearance (full car width), pick a bypass side (the side with the larger gap); leave margin for the car's width |
| Bypass | Leave the line, travel round without line guidance, keep observing until passed |
| After bypass | Rejoin the original line and resume |
| U-turn detail | At the junction, brake, pause approximately 200 ms, turn 180 degrees and track the return branch; never undo the turn (user request; pause reference pending D02/D08) |
| No pending barcode at junction | Continue straight with requested minimum 25 mm crossing travel; physical clearance/tolerance pending D07 |
| End of run | Endless healthy operation; local GP21 or a stopping guard ends movement, no automatic finish marker |

### Course rules

| Topic | Rule |
| --- | --- |
| Line sensing | Two analogue IR sensors follow the line (GP26/GP27); a third digital IR sensor reads barcodes (GP16). Three IR sensors is the maximum. Mounting is the team's choice. |
| Barcode placement | A barcode always precedes a junction; act at the next cross junction |
| Sides | Barcodes always on the left; humps always on the right (Nicholas confirmation 2026-10-29). Sensor placement on a reverse traversal still needs testing. |
| Humps | Single-wheel humps (no double humps). Final ramp is about the lower practice ramp's height, slightly lower or higher, and no more than 30 mm; the higher practice ramp is disregarded (course update 2026-10-27; 30 mm limit confirmed by Nicholas). The task is estimating height through IMU sensing and processing, not mechanical clearance. Measuring while moving scores better than stopping. Height is estimated from tilt/IMU data mapped to known heights by practical calibration, then tuned; the speed may be reduced after detection to allow measurement |
| Obstacles | Static, block the line; width 100-200 mm in supplied professor clarification. Straight-only placement is the user-selected operating assumption; inconsistent earlier source wording remains D12. |
| Map | Undisclosed until demonstration |

### Mission-day barcode rule

Confirmed by Nicholas 2026-10-14: alphabet A-Z; the instructor announces exactly four distinct letters on mission day (left, right, straight, U-turn); no recompilation afterwards. The four assignments load over WiFi as runtime configuration.

- All A-Z patterns are stored on the Pico; the mapping from letter to action is the only runtime input. Default test mapping per the briefing: A forward, B left, C right, D U-turn; the mission-day letters differ but the actions remain forward, left, right, U-turn at a cross junction.
- Firmware boots with every letter unmapped and refuses to start a run until a valid four-letter mapping is loaded; the other 22 letters stay unmapped.
- A reboot clears the mapping; the operator resends it before the next attempt.
- Example mappings are not official. Tests cover the written-brief example (A left, B right, C straight, D U-turn), the briefing example (A straight, B left, C right, D U-turn) and the briefing demo-day example (A straight, Z U-turn, W left, F right).

## 2. Subsystem deliverables

| Area | Deliverables / work |
| --- | --- |
| S1 Communications | Wi-Fi/MQTT API, telemetry framework, topic/message documentation, telemetry demonstration |
| S2 Motion | Motor drivers, encoders, speed/distance estimates, per-car PID wheel-speed tuning (equal PWM does not give equal speed; finalise wheels/mechanics before tuning), straight correction, repeatable distance/angle actions (45 and 90 degrees were examples); control library, PID tuning report, accuracy evaluation |
| S3 Line / Barcode | Three IR sensors, calibration, line position and junction logic, barcode decoder (A-Z, either direction), navigation command interface |
| S4 Terrain | GY-511 IMU driver, calibration against known heights/tilts, tilt detection, motion classification, collision detection, turn-rate monitoring, hump detection and peak measurement, event reporting, terrain analysis report |
| S5 Obstacle | Coarse scan then focused fine scan (example angles 30, 60, 90, 120, 150 degrees); profile generator; avoidance (stop, continue, turn left/right, reverse, reattempt); original-line recovery |
| Team | Defined inputs/outputs/owners, repeatable subsystem checks, measured calibration, early and progressive integration, integrated demonstration |

Turn rate and heading come from encoder differential motion; impact from the accelerometer. Briefing: heading/bearing can combine wheel-encoder and IMU data; combining sensors (sensor fusion) improves accuracy and is marked higher than using one sensor alone (GY-511 = accelerometer plus magnetometer, not a gyroscope); magnetometer-encoder fusion is associated with better marks but is a stretch goal, not a requirement. No height formula, tolerance or fusion algorithm was specified.

## 3. Communications

- Two-way MQTT: the robot publishes telemetry, returns a snapshot or summary on request and accepts the barcode mapping. Networking never drives, starts or stops the robot.
- On demonstration day the robot connects to the instructor's broker (a laptop broker is for development). Broker address, credentials, topics and schema were not supplied (D09).
- Telemetry: robot state, motor speed, encoder counts/distance, line sensor status, faults, barcodes with decoded command, hump count and heights with maximum, obstacle width/depth, current movement direction (straight, left, right) and decoded barcode messages. Messages must be readable and clear to a PC subscriber. An event record gives when and where (time and encoder position) barcodes, humps, obstacles and turns occurred, so the approximate route can be reconstructed. No map visualisation or localisation was prescribed.
- Payloads are compact binary (section 4). The laptop decoder (`scripts/mqtt_client.py`) prints readable text and derives motor speed and movement direction from successive encoder counts; a fancy dashboard is not required.
- Gap (requirement vs. earlier S1 scope): the briefing requires WiFi barcode-mapping configuration; an earlier S1 guide allowed status requests only.

## 4. Change in requirement: minimum-bit data representation

Change requirement one, adopted at Tribunal 1 (Week 8). Implemented in firmware before the final demonstration; the Week 13 submission carries its evidence.

**Requirement.** Each application data field uses only the bits its valid range needs, via C bit-fields or explicit masks and shifts (e.g. range 1-5: 3 bits; up to 8 states: 3 bits). Fault conditions (motor stall, encoder fault, IMU fault, no echo, Wi-Fi loss) are individual bits of a shared fault bitmask, not separate integers. Scope: motion/encoder; line/barcode; obstacle; hump estimation; vehicle and controller states; task-to-task and ISR-to-task data; inter-task queue messages; MQTT telemetry and command payloads (compact binary fields, not text values). Read-modify-write of packed data shared between tasks, cores or an ISR and a task must be race-free (atomic access or synchronisation).

**Briefing clarifications.** Loop counters and temporaries are exempt. Small fields may be combined into one integer or char, but do not combine merely because widths match; each owner justifies every width and provides accessor functions that read and write packed fields. Screen sharing in the discussion recording is for illustrating points; reading AI output aloud is not required.

**Week 13 evidence (in the repository):**

1. Data representation table (section 5): field, valid range, bits, representation.
2. RAM usage of the final implementation.
3. Message size of the final implementation (inter-task and/or communication messages).
4. Evidence of no lost functionality and no unacceptable timing or performance degradation.

Comparison of the revised implementation with the original is not required (course update, 2026-10-27). Evidence covers the final implementation only.

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

Source distinction: historical evidence rows retain their pinned source. Current
representation rows also contain local-checkpoint changes, not necessarily remote
main. In particular remote `5e6a45b` caps command speed at 300 mm/s; local
`4b00e20` uses 500 mm/s and adds CROSSING state/tuning. Neither speed is accepted
physical calibration or a new target. The target seven-state row is explicitly
proposed; all encoding changes require compatibility review.

Rules for every row:

- Bits = ceil(log2(valid values + invalid codes)).
- Combine fields into one storage unit only when they share a writer.
- Wire format uses explicit masks and shifts; in-RAM packed state may use bit-fields behind accessors.
- Stored floating-point and timestamp values need a bit-budget justification or fixed point.
- Loop counters and short-lived temporaries are exempt.
- **pending** = owner has not supplied the value (nothing assumed). **proposed** = drafted from the named issue branch, not signed off until the owner confirms in the issue #28 thread. This file records no approval that has not been given. Layout is reviewed jointly before any field is converted.

All rows are unsigned and rounding "none" unless stated. Columns: field | storage / writer | range (unit) | scale, rounding | invalid code, overflow | bits | representation.

### S1 Communications (proposed; frame layouts in architecture; needs Shaun's sign-off)

| Field | Storage / writer | Range | Invalid / overflow | Bits | Repr. |
| --- | --- | --- | --- | --- | --- |
| Header version | byte 0 bits 7-4, S1 | 2 | other rejected | 4 | mask/shift |
| Header type | byte 0 bits 3-0, S1 | 1-6 | other rejected | 4 | mask/shift |
| Sequence | byte 1, S1 | 0-255 | wraps mod 256 | 8 | byte |
| Mission state / motion status / line status | byte 2 | 0-15 / 0-3 / 0-3 | out of range rejected on encode and decode | 4 / 2 / 2 | mask/shift |
| Action ID / mapped | byte 3 | 0-63 / flag | bit 7 reserved | 6 / 1 | mask/shift |
| Fault bytes | bytes 4-9 | one byte per owner (six owners) | - | 6 x 8 | bytes |
| Encoder counts | bytes 10-13 | wrapped counts | wraps (modular) | 2 x 16 | little-endian |
| Hump count / max height | bytes 14-15 | 0-255 / 0-126 (D06 scale) | 127 invalid, bit 7 valid flag (any hump completed); count and height saturate | 8 / 7+1 | mask/shift |
| Front range | bytes 16-17 | 0-8190 mm, nearest mm | 8191 invalid, bit 13 valid flag; saturates to invalid | 13+1 | mask/shift |
| Last barcode letter / action | byte 18 | 0-25 or 31 none / 0-7 | 31 = no letter | 5 / 3 | mask/shift |
| Obstacle width / depth | bytes 19-22 | 0-65535 mm | 0 = none yet; saturates | 2 x 16 | little-endian |
| Event record: kind / arg / time / position / arg2 | 11-byte schema-v2 MQTT frame, S1 | kind 1-4; arg is 0-25 letter, 0-127 height code, or 0-7 action; time low 16 bits ms; two encoder counts; arg2 0-65535 obstacle depth or action | kind 0 and 5-15 rejected; time/counts wrap; obstacle depth saturates | 4 / 8 / 16 / 2x16 / 16 on wire; kind uses four low bits of byte 1 | explicit little-endian masks/shifts; **unchanged** (#112) |
| S1 RAM event entry: kind/arg/time/encoder counts/arg2 | `s1_packed_event_t` in event ring, S1 | kind 1-4, arg 0-127, remaining fields 0-65535 | invalid kind/arg rejected without changing ring; time/counts wrap | 3 / 7 / 16 / 2x16 / 16 = 74 logical bits; 80 stored (10 bytes incl. 6 spare bits) | 16-bit kind/arg field + 4x16-bit fields, accessor pack/unpack (#112) |
| S1 event ring indices | `s1_event_log_t.indices`, S1 | head 0-15; count 0-16 | valid operations maintain ranges | 4 / 5 = 9 logical bits; 16 stored (7 spare bits) | masks/shifts in internal accessors (#112) |
| S1 event ring drop count | `s1_event_log_t.dropped`, S1 | 0-4294967295 drops | saturates at max | 32 | `uint32_t`, count saturates (#112) |
| S1 event ring storage | `s1_event_log_t`, 16 entries | 16x10-byte entries + packed indices + 32-bit drop counter | target compiler layout asserted; current target probe recorded in SYS-12 | 1344 stored bits (168 bytes, including 16 padding bits) | `sizeof` asserted; probe `testing/subsystem1/sizeof_probe.c` (#112) |
| Mapping acknowledgement {sequence, result} | 3-byte frame | sequence 0-255; result 0-3 | result above 3 rejected; sequence wraps | 8 / 8 (2 used) | bytes |

### S2 Motion (encoder width provisional until D03; one constant `ENC_COUNT_BITS` in `robot_config.h`; branches #29, #31)

| Field | Storage / writer | Range | Invalid / overflow | Bits | Repr. |
| --- | --- | --- | --- | --- | --- |
| Encoder count per wheel | wrapping counter, encoder ISR (core 1) | wrapped counts; mm per count from D03 geometry | wraps; consumers use modular subtraction | 16 (provisional) | fixed-width |
| Motion command: mode/speed/steering/target/action/time | `MotionCommand`, Mission writer -> S2 mutex copy | mode 0-3; speed 0-500 whole mm/s (MOTION_SPEED_MAX_MMPS); steering -127..127 Q7; target -2000..2000 whole mm (DISTANCE) or -360..360 whole degrees (TURN); ID 0-63; sampled time uint32 ms | invalid constructor returns STOP; invalid packed/reserved fields and expired sample brake; target bound **provisional D07** | 2 / 9 / 8 / 16 / 6 / 32 logical = 73 bits; 96 stored (`sizeof` 12 bytes) | 16-bit speed+ID, 16-bit mode+Q7 steering, 16-bit target, 32-bit sampled time; accessor-only; #109 |
| Motion health: service time / sequence / stopped / brake ack | `MotionHealth`, S2 writer guarded by command mutex | times and sequence uint32; flags yes/no | sequence/time wrap; brake acknowledgement only after serviced stop | 32 / 32 / 1 / 1 logical = 66 bits; 96 stored (`sizeof` 12 bytes) | 2 flags in one uint8, 2x uint32; accessors; #109 |
| Motion feedback: status, action ID | feedback word, Motion | status 4 values (IDLE/RUNNING/DONE/FAILED); ID 0-63 | ID wraps; ID 1 reserved for FOLLOW | 2 + 6 (ID width provisional) | one word, single store |

### S3 Line, barcode and Mission (branches #35, #36, #37; pending rows converted by #52 after Nicholas confirms)

| Field | Storage / writer | Range | Invalid / overflow | Bits | Repr. |
| --- | --- | --- | --- | --- | --- |
| Mapping letters (L, R, S, U) | mapping word, Mission on valid mapping | A-Z, 26 values, all four distinct | boot state UNMAPPED; rejected before install | 5 each, 20 total | 4 x 5-bit |
| Mapping state | mapping word bit 20 | UNMAPPED / MAPPED | - | 1 | flag |
| Line sensor readings (2 analog) | S3 snapshot struct | 0-4095 ADC counts; fractions 0..1 as 0..127; error and steering -1..+1 as -127..+127 | validity flags; averaged ADC saturates at 4095; fractions clamp to 0..127 | 12 each; 7 each; 8 each | struct; scaled integers ([ADR 0001](adr/0001-base-2-scaled-integers.md)) |
| Barcode edge ring entry | ISR ring, barcode ISR | counts 2 x 16 (wrapped encoder counts at the edge); level 1 | overflow sets fault bit; new edge dropped | 16+16+1 | struct |
| Decoded barcode event | queue (8 deep), S3 | letter 0-25; action 0-4; orientation 0-1; position (left/right counts) | unmapped action 0; queue full sets fault bit, newest dropped | 5+3+1 (one 16-bit word) + 16+16 | packed word + 2 counts, accessors in `barcode_decode.h` |
| Mission state | Mission writer | READY, FOLLOW_LINE, NAVIGATE, SCAN, BYPASS, REACQUIRE, STOPPED; implemented codes 0..6, 7 invalid | invalid input inhibits movement | 3 logical (existing 4-bit wire slot retained) | Implemented 3-bit private state and atomic published byte; schema-v2 wire slot remains 4 bits; decoder uses seven-state labels |
| Broad-black encounter start | `Mission.broad_since_ms`, Mission writer | uint32 milliseconds, modulo 2^32 | meaningful only while `in_broad`; unsigned elapsed subtraction handles wrap | 32 | private timestamp; no wire format |
| Existing Safety latched cause | current Safety task | 4 cause bits (GP21, fault, stale line, stale motion) | 0 = not latched | 4 | Existing implementation only; retire task after equivalent Motion checks; target causes/width reviewed before conversion |

### S3 bit-usage data contract (proposed; spec #100; final evidence #107)

Historical source-pinned storage evidence below is retained unchanged; it is not a size prediction for the restructure. Storage bits come from `testing/subsystem3/sizeof_probe.sh` (accepts an optional probe source; e.g. `testing/subsystem1/sizeof_probe.c`, arm-none-eabi-gcc 15.3.1, target flags of `build_make/pico_rp2040.mk`); #107 cross-checks struct totals against the linked `.bss` objects in `arm-none-eabi-size`. Instances count persistent copies only; per-iteration local copies are exempt. Each chosen width is justified by the bounds in its row: counts follow `ENC_COUNT_BITS` (16, provisional until D03); fractions (0..1 as 0..127) and error/steering (-1..+1 as -127..+127) follow [ADR 0001](adr/0001-base-2-scaled-integers.md) (retained representation decision); raw IR follows the 12-bit ADC (0-4095); letter/action/orientation follow the decoded-barcode row above. The ticket in the last column implements the width; #107 records final storage bits from the probes. No comparison with the original implementation is required.

| Field | Current C type | Bounds / units | Chosen width (bits) | Final storage bits | Instances | Ticket |
| --- | --- | --- | ---: | ---: | ---: | --- |
| Barcode edge: sample time (`sampled_us`) | deleted (was `uint32_t`) | Never read | 0 (deleted) | 0 | 64 | #102 |
| Barcode edge: left / right count | 2 x `barcode_count_t` (`uint16_t`) | Encoder counts, `ENC_COUNT_BITS` (16, provisional until D03) | 2 x 16 | 2 x 16 | 64 | #103 |
| Barcode edge: level (`high`) | `bool` | Boolean | 1 | 8 | 64 | - |
| Barcode edge: padding | - | - | - | 8 | 64 | #102, #103 |
| Barcode edge struct | `barcode_edge_t` | - | - | 48 | 64 | - |
| Barcode event: left / right count | 2 x `barcode_count_t` (`uint16_t`) | Encoder counts, `ENC_COUNT_BITS` (16, provisional until D03) | 2 x 16 | 2 x 16 | 8 | #103 |
| Barcode event: letter | bits 0-4 of `packed` (`uint16_t`) | Letter 0-25; 31 = no letter | 5 | 5 | 8 | #104 |
| Barcode event: action | bits 5-7 of `packed` | Action 0-4 | 3 | 3 | 8 | #104 |
| Barcode event: orientation | bit 8 of `packed` | Orientation 0-1 | 1 | 1 | 8 | #104 |
| Barcode event: padding | bits 9-15 of `packed` (spare) | - | - | 7 | 8 | #104 |
| Barcode event struct | `barcode_event_t` | - | - | 48 | 8 | - |
| Barcode frame: previous left / right count | 2 x `barcode_count_t` (`uint16_t`) | Encoder counts, `ENC_COUNT_BITS` (16, provisional until D03) | 2 x 16 | 2 x 16 | 1 | #103 |
| Barcode frame struct | `barcode_frame_t` | - | - | 1248 | 1 | - |
| IR reading (in tracker): left / right fraction | 2 x `uint8_t` | 0..1 as 0..127 (ADR 0001) | 2 x 7 | 2 x 8 | 1 | #105 |
| IR reading (in tracker): error | `int8_t` | -1..+1 as -127..+127 (ADR 0001) | 8 | 8 | 1 | #105 |
| IR tracking output: error / steering | 2 x `int8_t` | -1..+1 as -127..+127 (ADR 0001) | 2 x 8 | 2 x 8 | 0 (returned by value) | #105 |
| IR tracker: raw left / right | bits 0-23 of `IrRawWord` (`uint32_t`) | 12-bit ADC, 0-4095 | 2 x 12 | 2 x 12 | 1 | #106 |
| IR tracker: sample valid | bit 24 of `IrRawWord` (bits 25-31 spare) | Boolean | 1 | 1 (+7 spare) | 1 | #106 |
| IR tracker struct | `IrTracker` | - | - | 480 | 1 (S3 task stack) | - |
| IR snapshot: left / right fraction | 2 x `uint8_t` | 0..1 as 0..127 (ADR 0001) | 2 x 7 | 2 x 8 | 1 | #105 |
| IR snapshot: error / steering | 2 x `int8_t` | -1..+1 as -127..+127 (ADR 0001) | 2 x 8 | 2 x 8 | 1 | #105 |
| Mission state (published copy) | `_Atomic uint8_t` (was `uint32_t`) | 8 states | 3 | 8 (smallest atomic store) | 1 | #105 |
| IR snapshot: raw left / right | bits 0-23 of `IrRawWord` (`uint32_t`) | 12-bit ADC, 0-4095 | 2 x 12 | 2 x 12 | 1 | #106 |
| IR snapshot: acquisition valid | bit 24 of `IrRawWord` (bits 25-31 spare) | Boolean | 1 | 1 (+7 spare) | 1 | #106 |
| IR snapshot struct | `S3IrSnapshot` | - | - | 320 | 1 | - |

Single-store check (#106, 2026-10-27): `arm-none-eabi-objdump -d` of `ir_tracking.o` (arm-none-eabi-gcc 15.3.1, offline normal profile) shows both writes of `IrTracker.raw` in `ir_tracking_sample` as one `str` to the aligned word; the snapshot copies the word whole. Removing the two `bool` flags freed bytes that alignment padding absorbs, so `IrTracker` (60 bytes) and `S3IrSnapshot` (40 bytes) keep their size.

The final build (section-4 command) and the probe-to-object cross-check are recorded by #107 as S3-M01 in the [S3 test log](../testing/subsystem3/TEST_LOG.md#7-minimum-bit-final-memory-s3-m01--2026-10-27). On-target S3 loop timing (S3-M02) and the physical line-follow run need the robot and are tracked with the [mission-readiness checklist](../testing/mission-readiness.md).

### S4 Terrain (scale provisional, 1 mm per count `HUMP_MM_PER_COUNT`, until D06; branches #45, #46)

| Field | Storage / writer | Range | Invalid / overflow | Bits | Repr. |
| --- | --- | --- | --- | --- | --- |
| Hump height, working | task-local | 0-6553.5 mm, 0.1 mm, nearest | saturates at 65535 | 16 | fixed point |
| Hump height, reported | terrain word bits 0-6 | 0-126 (D06 scale), nearest | 127 invalid; saturates at 126 | 7 | fixed point |
| Maximum hump height | terrain word bits 7-13 | 0-126, same scale | 127 invalid; saturates at 126 | 7 | fixed point |
| Hump count | terrain word bits 14-21 | 0-255 | saturates at 255 | 8 | unsigned |
| Terrain state / height valid / calibrated | terrain word bits 22-26 | 5 states / flag / flag | - | 3 / 1 / 1 | mask/shift |
| Persistent acceleration filter | `TerrainContext.filtered_accel_q16`, one S4 task writer | 3 signed raw-accelerometer axes with 16 fractional bits; widened bias-corrected range | saturates on int32 overflow | 3 x 32 | Q16 signed integers; local float calculations only (#110) |
| Calibration accumulation | `TerrainContext.calib_sum`, S4 task | 100 x signed 16-bit raw samples per axis, -3276800..3276700 | cannot overflow signed 23 bits during 100 samples | 3 x 23 = 69 bits; 72-bit storage | GNU C packed 23-bit signed bit-fields (#110) |
| Pitch estimate | `TerrainContext.pitch_cdeg`, S4 task | -9000..9000 centidegrees | rounded nearest 0.01 degree | 15 logical, 16 stored | `int16_t` (#110) |
| Terrain counters | `calib_count` / `flat_run`, S4 task | 0-100 / 0-10 | calibrated stops count at 100; flat run resets at 10 | 7 / 4 | C bit-fields (#110) |
| Terrain private state / validity flags | `TerrainContext.state`, `height_valid`, `is_calibrated` | 0-4 / flag / flag | state 5-7 unused | 3 / 1 / 1 | C bit-fields (#110) |
| Terrain private hump last/max codes | `TerrainContext.last_hump_code`, `max_hump_code` | 0-126; 127 invalid | saturates and retains max | 7 / 7 | C bit-fields (#110) |
| Complete persistent TerrainContext | `TerrainContext`, S4 task | as above; 20 ms processing | D06 physical calibration pending; replay old float path for parity | 32 bytes on the current target compiler (SYS-12) | `testing/subsystem4/sizeof_probe.c` (#110) |

### S5 Obstacle (proposed; branches #48, #49)

| Field | Storage / writer | Range | Invalid / overflow | Bits | Repr. |
| --- | --- | --- | --- | --- | --- |
| Scan state | scan word bits 0-2, Obstacle | 8 states (IDLE, SETTLING, COARSE, FINE, DONE, FAILED, CANCELLED, SIDE_WATCH) | out-of-range decodes as FAILED | 3 | encoded value |
| Scan ID | scan word bits 3-6, Mission request | 0-15 | wraps | 4 | mask/shift |
| Range, front / side | echo word, Obstacle | 0-8190 mm, nearest mm | 8191 or valid flag clear = no echo, never clear space; saturates to invalid | 13+1+16 (time low 16) | mask/shift |
| Obstacle profile (geometry) | mutex-copied `ObstacleProfile`, S5 | closest angle -20..+20, valid flag; 4 unsigned mm measurements 0-65535; 2 signed mm coordinates | clamp to 16-bit limits; invalid profile not published | angle 6 + valid 1 + 6x16 = 103 logical; 112 stored (`sizeof` 14 bytes) | signed angle byte, validity bit in a separate byte, other 16-bit words |
| ObstacleRay scan entry | `ObstacleRay` array, private S5 task | angle -20..+20; valid 0/1; range mm 0-65535 | invalid angle rejected by constructor, no echo = validity clear; range ignored | 6 + 1 + 16 = 23 logical bits; 32 stored bits (`sizeof` 4 bytes) | GNU C packed signed angle byte, validity byte and unaligned 16-bit range |
| Obstacle width / depth | profile width; depth from encoder travel | 0-65535 mm / counts | saturates | 16 | unsigned |
| Side-watch servo angle / enabled | `side_command` atomic word, Mission writer | -20..+20 degrees / yes-no | clamped to sweep; value sign-extended on read | 6 + 1 logical; 32 stored (shared atomic word) | bits 1..8 signed angle, bit 0 enabled; accessors (#111) |

### Mission and Safety

Existing storage below describes the pre-consolidation implementation; target safety functions and latch ownership are in [architecture](architecture.md#motion-command-contract).

| Field | Storage / writer | Range | Bits | Repr. |
| --- | --- | --- | --- | --- |
| Start-blocking / run-stopping check | read on demand (`rd_fault_start_blocked`, `rd_fault_stop_pending`), Mission / Safety (read only) | owner bytes masked by the policy in [architecture](architecture.md#fault-bitmask) | 1 each (result) | bool |
| Stop latch cause | cause word, Safety | 4 cause bits; 0 = not latched | 4 | bit mask |

### Fault bitmask (`app_program/common/robot_data.h`; see architecture for status)

One byte per subsystem owner, not one byte per target task; single-writer placement is owned by [architecture](architecture.md#fault-bitmask). Every bit is reported in telemetry. Which bits block a start or stop a run: [architecture](architecture.md#fault-bitmask).

| Byte | Writer | Bit 0 | Bit 1 | Bit 2 |
| --- | --- | --- | --- | --- |
| Motion | Motion | motor stall | encoder fault | - |
| Line | Mission (target) | IR invalid | barcode capture overflow | barcode event overflow |
| Terrain | I/O (target) | IMU fault | calibration failed | impact |
| Obstacle | Mission (target) | no echo | - | - |
| Mission | Mission task | line lost | - | - |
| Network | kernel task (current core 0; target placement in [architecture](architecture.md#tasks)) | Wi-Fi lost | broker lost | - |

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

Single register. Record answers and measured values here once, then link from other guides.

| ID | Decision / missing evidence | Owner |
| --- | --- | --- |
| D01 | Verify issued Code 39 framing/print scales, quiet zones, polarity, width tolerances and encounter rearm; both orientations must yield the same letter | S3 / Nicholas |
| D02 | Mounted junction confirmation/turn centre, barcode-to-junction offsets, multiple pending characters, no-command crossing qualification, U-turn pause reference (requested about 200 ms), bypass/reverse boundaries and failed-scan retry policy; endless/no undo is settled | Nicholas |
| D03 | Verify motor and encoder polarity, counts/revolution, ratings/levels, geometry, full car/turning envelope and maximum edge rate. Remote signs +1/+1 versus local -1/+1 are not a calibration decision; preserve barcode half-range bound | S2 / Nicholas |
| D04 | Mounted IR position/spacing/height, contrast/noise/lighting range and barcode black polarity; wiring user-confirmed, measured sensing margins still pending | S3 |
| D05 | Sonar electrical behavior user-confirmed; retain recorded Echo-voltage evidence when available. Calibrate repeat interval/range/servo centre/endpoints/settling, sample groups/median/edge hysteresis, coverage and width-estimation uncertainty | S5 |
| D06 | Stationary boot IMU calibration before IR: bounded success/failure handshake, axes/bias, impact/filter limits and reference-validated height method/error for humps <=30 mm. Confirm half-speed completion and proposed results-retained-on-stop/new-start-reset lifecycle | S4 / Nicholas |
| D07 | Wheel/line gains, speed/duty/distance/turn/stall limits; 25 mm crossing tolerance; exact moving +/-70 degree sweep sequence for two back-and-forth attempts, creep speed, travel budget and stable intended-line confirmation; bypass/depth/clearance budgets | Owners / Nicholas |
| D08 | Output-brake latency versus physical stop distance/time, measured stationarity and U-turn pause start; GP21 release-reset eligibility must not imply physical stationarity | S2 / Nicholas |
| D09 | Instructor WiFi/broker settings; normal mission requires both before starting, loss mid-run report-only. Decide whether professor buffer wording requires literal RTOS message buffers; existing bounded TX/RX rings retained provisionally | S1 / Nicholas |
| D10 | Agree first-cut lateral/turn/height/width error, recovery travel and repeatability targets before corresponding physical PASS; label proposals separately from course thresholds | Nicholas |
| D11 | Settled 2026-10-29: barcode left, hump right. Validate sensing after reverse/U-turn traversal without inventing mirrored lane placement | S3 / S4 |
| D12 | Straight-only obstacles are the user's operating assumption; confirm course applicability despite inconsistent earlier briefing wording; do not claim curved-course bypass support | Nicholas |
| D13 | Numeric target priorities/periods, execution budgets, timeout slack, data-age limits, worst sensor-to-PWM latency, GPIO ISR time, stack margin and load-test criteria; user authorized existing numeric settings as provisional software defaults on 2026-10-29; physical scheduling/age acceptance remains open | Owners / Nicholas |
| D14 | Core swap superseded. Audit core-0 I/O integration, cross-core event producers/mapping hand-off, synchronization and required initialization order; kernel WiFi/USB and capture core unchanged | Owners |
| D15 | Watchdog reset policy/scope, enabled-state audit and physical reset/output behavior; tick initialization is not proof of watchdog protection. No new framework in this restructure | Nicholas |
| D16 | Settled 2026-10-29: configurable 15 cm is the obstacle trigger distance (150 mm). It does not authorize any travel without valid forward sonar | Nicholas |

Implementation note (2026-10-29, #130): Mission's local reset handshake has
three exclusive states (boot, await GP21 press, await GP21 release), encoded as
0..2 in a private `uint8_t` (2 logical bits, 8 stored; no wire field). Unknown
state cannot request reset. Stop request/service counters remain wrapping
32-bit core-1 counters; reset requires equality inside the output exclusion
section. The software brake acknowledgement is not physical stationarity.
User authorized retaining existing timing/age constants as provisional defaults
for software implementation; D13 physical measurement/acceptance remains open.

Software restructure storage contract (2026-10-29, #131–#133; no new wire fields):

| Field / owner | Range and representation | Validity / overflow | Logical / stored bits |
| --- | --- | --- | --- |
| Mission state / navigation phase, Mission | State codes 0 READY, 1 FOLLOW_LINE, 2 NAVIGATE, 3 SCAN, 4 BYPASS, 5 REACQUIRE, 6 STOPPED; phases 0..5 | State 7 invalid -> stop; phases private, produced only by coordinator | 3 + 3 in private byte; published state remains atomic byte, existing 4-bit wire slot |
| Pending mapping, I/O | Existing four 5-bit letters + mapped bit; sequence 0..255 at bits 21..28 | Submit rejects invalid/duplicate letters and occupied hand-off | 29 / one 32-bit word |
| Mapping reply, Mission | Sequence bits 0..7; ACK result 0..3 at bits 8..9 | Reply only after Mission's install/reject decision | 10 / one 32-bit word |
| Mapping progress, one writer per counter | Request/reply tickets plus private submit/consume/take counters, modulo 2^32 | One outstanding request through reply consumption; no overwrite | 32 each |
| Terrain evidence, I/O | Existing terrain word, acquisition/service ms and attempt sequence uint32; acquisition validity 1 bit | Pending data does not renew acquisition time; copy/age/validity required; timestamps wrap | 4 x 32 + 1 logical; naturally aligned 20-byte snapshot |
| IMU startup outcome, I/O | 0 pending, 1 calibrated, 2 failed | Missing IMU publishes failed; never interpreted as calibrated | 2 / one 32-bit word |
| S5 step phase/flags, Mission | Phase 0..2 + initialized/ready/scanning; scan counts 0..9, scan ID 0..15, angle -20..20 | Fixed nine-ray capacity; deadline/cancel/movement abort; no echo invalid | 2+3 bits; counts 4 each, ID 4, signed angle 6 (stored in 8) |
| Link readiness, kernel WiFi | bit 0 WiFi, bit 1 broker; boot 0 | Both required for local start; run loss report-only | 2 / one 32-bit word |
| Event drops, event-mutex holder / one writer per core | `s1_event_log_t.dropped` counts ring overflow; `lock_misses[2]` counts failed bounded event-insertion locks (Mission core 1, Terrain core 0), one atomic uint32 per core; bridge-full returns false for retry | Each count saturates at UINT32_MAX; insertion waits at most the existing 2 ms snapshot-lock timeout; counters are debugger diagnostics, not new MQTT fields or delivery acknowledgements | 32 each; 64 stored for the two lock-miss words |

Mission private history conversion contract (2026-10-30; software precision,
not a physical tuning or accuracy claim; one Mission writer):

| Field | Range / scale / rounding | Invalid / overflow | Logical / stored bits |
| --- | --- | --- | --- |
| Filtered previous line error | -1..+1, signed integer /32767; nearest, ties away from zero | -32768 unused; primed bit gates history; finite local calculation clamps before storage | 16 / 16 |
| Integral steering contribution | +/-`LINE_I_LIMIT` (currently 0.40); signed integer /32767; nearest, ties away from zero | -32768 unused; reset zero; computation clamps to configured limit before storage; limit constrained to <=1 | 15 at current limit / 16; resolution chosen for host float-reference comparison, physical acceptance D07 pending |
| Held steering | -127..127, signed Q7; existing output rounding retained | -128 unused; output clamps to `MAX_STEER` | 8 / 8 |
| PID primed | no/yes, Mission private | reset false | 1 / 1 in a private byte |
| Junction turn | 0 none, 1 left (+90 degrees), 2 right (-90), 3 U-turn (+180) | none yields zero target (Motion rejects a zero turn); only coordinator sets codes | 2 / 2 in private byte |

Float remains only in local PID/filter/geometry computation, where existing
gains and rate limits use fractional seconds. Persistent PID history uses the
scales above; quantization is at storage boundaries, with no wire change.
The /32767 scale provides exact +/-1 endpoints and half-step storage error
<=1/65534. Host comparison against the prior float controller is software
evidence only; cumulative error and physical steering still require D07/T15.
Current working-tree target ABI, static RAM and inter-task copy sizes are
recorded in [SYS-12](../testing/system/TEST_LOG.md#current-resource-audit-sys-12).
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

User steering, 2026-10-29: use the pin allocation in team.md; recovery/filtering
operating settings and the proposed Terrain new-run reset lifecycle remain
pending. New helper configuration uses explicit caller-supplied values in host
fixtures. Unknown production settings are encoded as zero/invalid and stop the
corresponding manoeuvre; no provisional values are silently enabled.

Junction qualifier correction: Mission records the 32-bit timestamp of the last
qualification sample and two one-bit flags (black encounter still open, qualified)
beside its one-bit broad-encounter flag in one private byte. Only distinct usable
black acquisitions may qualify the existing encoder-distance threshold; forced
25 mm crossing travel after black has ended cannot qualify an earlier thin patch.
No mounted geometric tolerance is claimed by these software flags.

Continuation storage: `s5_range_sample_t` contains the existing packed word plus
full 32-bit acquisition time, sequence and wrapped mean encoder count at
range completion (16 bytes). The count uses the existing unsigned encoder-count
scale and wrap rules; Mission uses it to debit travel since the front reading. Mission owns its current
front/side pair; a separate pair is copied under the existing profile mutex for
optional core-0 reporting, preserving original times when publication is busy.
`s5_profile_snapshot_t` combines the 14-byte profile, six-byte edge/model evidence
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
| Recovery | 270 mm/s, steering magnitude 76/127, 30 s / 1200 mm total budget, 5 degree target tolerance and overshoot allowance | Revised trial after stalls at 30 and 90 mm/s; wheel targets match the current tracking curve range; D03/D07/D08/D13 pending |
| Recovery alignment | Original heading within 15 degrees; absolute error <=25/127 for three distinct acquisitions | D04/D07/D10 pending |
| Bypass line alignment | 30 mm/s, independently configured from standalone recovery | D04/D07/D10 pending |
| Scan | Three samples, at least two valid; depth enter/exit bands 30/60 mm; two adjacent rays; model range uncertainty 10 mm | D05/D10 pending; uncertainty is a proposed model input, not sensor accuracy |
| Servo sampling | User-requested +/-20 degree travel; coarse rays -20/-10/0/+10/+20 degrees and 5 degree edge refinement; existing 300 ms settle and 60 ms repeat | Verify endpoints/mounting before release, D05 |
| Bypass | 60 mm/s, <=50 mm motion chunks, 90 s / 4000 mm whole budget; <=2 s / two completed groups per sensing phase | D02/D03/D05/D07/D08/D13 pending |
| Bypass geometry | Body length/width 200/180 mm, 30 mm clearance; offset >=250 mm, <=600 mm; conservative obstacle width >=200 mm | Proposals only; measure full swept envelope, D03 |
| Rear-edge / return | Two distinct qualified side groups; 150/100 mm enter/release jump; <=1500 mm pass, <=250 mm baseline search; return within 25 mm lateral / 5 degrees and three distinct tracking acquisitions | D02/D05/D07/D10 pending |
| Height quality | Raw acceleration norm 900..1100 mg, wheel speed difference <=30 mm/s, speed change <=30 mm/s per fresh sample, 200 ms steady data before a height window | D06/D07/D10 pending; suppress unqualified height results |
| Retry / reverse | One stationary rescan; no autonomous reverse without rear-coverage support | Retry is a proposal; reverse remains unsupported |

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
At the user-requested +/-20 degree servo limits, angle needs six signed bits;
ray validity needs one more bit. Existing eight-bit angle storage is retained
to preserve the current layouts; commands and constructors enforce the sweep. `ObstacleRay` becomes four packed bytes; `ObstacleProfile`
remains 14 bytes because its previous alignment byte holds validity. Side-watch
uses eight angle bits plus one enable bit in its existing atomic word. Terrain
adds one window-quality bit beside its existing classification/result flags;
completed count/maximum/last results are never reset on accepted starts.
