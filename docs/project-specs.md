# Project specifications

Single owner of requirements, the change requirement (CR), the Week 13 data table and pending decisions. Sources: the written brief and the instructor briefings ("briefing" below). Discrepancies between sources were resolved with the team on 2026-10-25 and are recorded here. Week 8 realigned plan: [tribunal1](additional-docs/tribunal1.md). Design: [architecture](architecture.md). Members, hardware, workflow and coding rules: [team](team.md). Subsystem guides: [S1](architecture/subsystem1.md), [S2](architecture/subsystem2.md), [S3](architecture/subsystem3.md), [S4](architecture/subsystem4.md), [S5](architecture/subsystem5.md).

Status labels: **requirement** (source-stated), **proposed** (drafted, not signed off), **implemented** (in code), **observed** (measured). Rows below are requirements unless marked.

## Contents

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
  - [Fault bitmask (proposed by `app_program/common/robot_data.h`, issue #27; owners confirm)](#fault-bitmask-proposed-by-app_programcommonrobot_datah-issue-27-owners-confirm)
- [6. Assessment, demonstration and process rules](#6-assessment-demonstration-and-process-rules)
- [7. Course geometry](#7-course-geometry)
- [8. Not specified by any source](#8-not-specified-by-any-source)
- [Pending decisions](#pending-decisions)

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

### Course rules

| Topic | Rule |
| --- | --- |
| Line sensing | Two analogue IR sensors follow the line (GP26/GP27); a third digital IR sensor reads barcodes (GP16). Three IR sensors is the maximum. Mounting is the team's choice. |
| Barcode placement | A barcode always precedes a junction; act at the next cross junction |
| Sides | Barcodes lie on one fixed side of the line and humps on the other (never the same side); the instructor said the sides would be confirmed when the kit is issued; which side is D11 |
| Humps | Single-wheel humps of varying heights (no double humps). Measuring while moving scores better than stopping. Height is estimated from tilt/IMU data mapped to known heights by practical calibration, then tuned; the speed may be reduced after detection to allow measurement |
| Obstacles | Static, block the line. Briefing says straight sections in earlier passages and "straight and curved" in one later sentence: D12 |
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

Compile and link results only; no firmware was executed or flashed. Measured from pinned commits: `main` `97de812`, `uat` `d8220bc`, both with the same `build_make` makefiles (taken from `main` `90628ba`). Toolchain: arm-none-eabi-gcc 15.3.1, Pico SDK 2.3.1, macOS host. Command in each `build_make/`: `make clean`, then `make -k -j8 SMP=1 CONSOLE=usb_cdc WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=0`, then `arm-none-eabi-size mtk3pico_smp1_usb_cdc.elf`.

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

Rules for every row:

- Bits = ceil(log2(valid values + invalid codes)).
- Combine fields into one storage unit only when they share a writer.
- Wire format uses explicit masks and shifts; in-RAM packed state may use bit-fields behind accessors.
- Stored floating-point and timestamp values need a bit-budget justification or fixed point.
- Loop counters and short-lived temporaries are exempt.
- **pending** = owner has not supplied the value (nothing assumed). **proposed** = drafted from the named issue branch, not signed off until the owner confirms in the issue #28 thread. This file records no approval that has not been given. Layout is reviewed jointly before any field is converted.

All rows are unsigned and rounding "none" unless stated. Columns: field | storage / writer | range (unit) | scale, rounding | invalid code, overflow | bits | representation.

### S1 Communications (proposed; frame layouts in [architecture](architecture.md); needs Shaun's sign-off)

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
| S1 event ring storage | `s1_event_log_t`, 16 entries | 16x10-byte entries + packed indices + 32-bit drop counter | target compiler layout asserted; target probe pending | 1344 stored bits (168 bytes, including 16 padding bits) | `sizeof` asserted; probe `testing/subsystem1/sizeof_probe.c` (#112) |
| Mapping acknowledgement {sequence, result} | 3-byte frame | sequence 0-255; result 0-3 | result above 3 rejected; sequence wraps | 8 / 8 (2 used) | bytes |

### S2 Motion (encoder width provisional until D03; one constant `ENC_COUNT_BITS` in `robot_config.h`; branches #29, #31)

| Field | Storage / writer | Range | Invalid / overflow | Bits | Repr. |
| --- | --- | --- | --- | --- | --- |
| Encoder count per wheel | wrapping counter, encoder ISR (core 1) | wrapped counts; mm per count from D03 geometry | wraps; consumers use modular subtraction | 16 (provisional) | fixed-width |
| Motion command: mode/speed/steering/target/action/time | `MotionCommand`, Mission writer -> S2 mutex copy | mode 0-3; speed 0-300 whole mm/s; steering -127..127 Q7; target -2000..2000 whole mm (DISTANCE) or -360..360 whole degrees (TURN); ID 0-63; sampled time uint32 ms | invalid constructor returns STOP; invalid packed/reserved fields and expired sample brake; target bound **provisional D07** | 2 / 9 / 8 / 16 / 6 / 32 logical = 73 bits; 96 stored (`sizeof` 12 bytes) | 16-bit speed+ID, 16-bit mode+Q7 steering, 16-bit target, 32-bit sampled time; accessor-only; #109 |
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
| Mission state | state value, Mission | 7 states (CALIBRATE, READY, FOLLOW, JUNCTION_TURN, BYPASS, COMPLETE, STOP) | - | 3 (4 on wire) | encoded value |
| Safety latched cause | cause word, Safety | 4 cause bits (GP21, fault, stale line, stale motion) | 0 = not latched | 4 | bit mask |

### S3 bit-usage data contract (proposed; spec #100; final evidence #107)

Storage bits come from `testing/subsystem3/sizeof_probe.sh` (accepts an optional probe source; e.g. `testing/subsystem1/sizeof_probe.c`, arm-none-eabi-gcc 15.3.1, target flags of `build_make/pico_rp2040.mk`); #107 cross-checks struct totals against the linked `.bss` objects in `arm-none-eabi-size`. Instances count persistent copies only; per-iteration local copies are exempt. Each chosen width is justified by the bounds in its row: counts follow `ENC_COUNT_BITS` (16, provisional until D03); fractions (0..1 as 0..127) and error/steering (-1..+1 as -127..+127) follow [ADR 0001](adr/0001-scaled-integers-base-2.md) (on branch `docs/minimum-bit-s3`, not yet merged); raw IR follows the 12-bit ADC (0-4095); letter/action/orientation follow the decoded-barcode row above. The ticket in the last column implements the width; #107 records final storage bits from the probes. No comparison with the original implementation is required.

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
| Mission state (published copy) | `_Atomic uint8_t` (was `uint32_t`) | 7 states | 3 | 8 (smallest atomic store) | 1 | #105 |
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

### S5 Obstacle (proposed; branches #48, #49)

| Field | Storage / writer | Range | Invalid / overflow | Bits | Repr. |
| --- | --- | --- | --- | --- | --- |
| Scan state | scan word bits 0-2, Obstacle | 8 states (IDLE, SETTLING, COARSE, FINE, DONE, FAILED, CANCELLED, SIDE_WATCH) | out-of-range decodes as FAILED | 3 | encoded value |
| Scan ID | scan word bits 3-6, Mission request | 0-15 | wraps | 4 | mask/shift |
| Range, front / side | echo word, Obstacle | 0-8190 mm, nearest mm | 8191 or valid flag clear = no echo, never clear space; saturates to invalid | 13+1+16 (time low 16) | mask/shift |
| Quantized obstacle profile | mutex-copied snapshot | distances 0-65535 mm; closest angle -60..+60 (signed) | valid flag clear; saturates at 16 bits | 5 x 16 + 1 | struct; packed pending (#54) |
| Obstacle width / depth | profile width; depth from encoder travel | 0-65535 mm / counts | saturates | 16 | unsigned |
| Servo angle | command, Obstacle | -60..+60 degrees (signed) | clamped to sweep limit | 7 | signed |

### Mission and Safety

| Field | Storage / writer | Range | Bits | Repr. |
| --- | --- | --- | --- | --- |
| Start-blocking / run-stopping check | read on demand (`rd_fault_start_blocked`, `rd_fault_stop_pending`), Mission / Safety (read only) | owner bytes masked by the policy in [architecture](architecture.md#fault-bitmask) | 1 each (result) | bool |
| Stop latch cause | cause word, Safety | 4 cause bits; 0 = not latched | 4 | bit mask |

### Fault bitmask (proposed by `app_program/common/robot_data.h`, issue #27; owners confirm)

One byte per owner task; each owner sets and clears only its own bits. Every bit is reported in telemetry. Which bits block a start or stop a run: [architecture](architecture.md#fault-bitmask).

| Byte | Writer | Bit 0 | Bit 1 | Bit 2 |
| --- | --- | --- | --- | --- |
| Motion | S2 task | motor stall | encoder fault | - |
| Line | S3 task | IR invalid | barcode capture overflow | barcode event overflow |
| Terrain | S4 task | IMU fault | calibration failed | impact |
| Obstacle | S5 task | no echo | - | - |
| Mission | Mission task | line lost | - | - |
| Network | kernel task, core 0 | Wi-Fi lost | broker lost | - |

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

Finish marker and end-of-mission stop; feature counts, order or route; handling of multiple barcodes before a junction; barcode symbology, framing, tolerances; obstacle dimensions and bypass boundaries; numerical accuracy tolerances; time penalties and weights; broker address, credentials, topics, payload schema.

## Pending decisions

Single register. Record answers and measured values here once, then link from other guides.

| ID | Decision / missing evidence | Owner |
| --- | --- | --- |
| D01 | Issued barcode symbology/framing, quiet zones, width tolerances and duplicate rearm | S3 / team |
| D02 | Finish convention, junction/crossing rules, barcode-to-junction spacing, print scale and permitted bypass/reverse boundaries | Team |
| D03 | Motor/encoder ratings and logic levels, decoded counts per wheel revolution, polarity, wheel diameter, track width and car envelope Barcode bound: the longest barcode (29 x `BARCODE_MAX_ELEMENT_COUNTS`) must stay below half the count range; a static assertion in `barcode_decode.h` re-checks it when `ENC_COUNT_BITS` changes | S2 |
| D04 | Mounted IR module levels, polarity, height, contrast/noise and lighting range | S3 |
| D05 | Installed ultrasonic model and Echo level (<= 3.3 V before connecting GP2), range, repeat interval and no-echo behaviour; servo supply, endpoints, centre and settling | S5 |
| D06 | IMU supply/pull-ups, axes/bias, filters, impact thresholds and height method against reference humps | S4 |
| D07 | Wheel PID gains, speed/turn accuracy, stall/clearance/stopping thresholds, scan/reacquisition limits and retries | Owners / team |
| D08 | Measured stop latency and stationary-detection threshold | S2 / Safety |
| D09 | Instructor network/broker settings available before program freeze; network/stack load | S1 / team |
| D10 | Numerical acceptance thresholds, Week 10 demonstration format/date, instructor repository access and evidence schedule | Team |
| D11 | Which side of the line holds barcodes and which holds humps | Team |
| D12 | Whether obstacles appear only on straight sections (the briefing states both) | Team |
| D13 | Required timing margin for each final-implementation loop below its configured period (e.g. S3 input 10 ms, IR ADC sampling 20 ms) | Each owner / team |
