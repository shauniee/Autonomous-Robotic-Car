# Team, hardware and workflow

## Contents

- [1. Members and subsystems](#1-members-and-subsystems)
- [2. Pins](#2-pins)
  - [Supply and wires](#supply-and-wires)
  - [Wiring conflicts](#wiring-conflicts)
  - [Physical header map (W/WH; header numbers differ from GPIO numbers)](#physical-header-map-wwh-header-numbers-differ-from-gpio-numbers)
- [3. Task priorities and kernel object budget](#3-task-priorities-and-kernel-object-budget)
- [4. Interfaces between subsystems](#4-interfaces-between-subsystems)
- [5. Coding rules](#5-coding-rules)
- [6. Data representation](#6-data-representation)
  - [Rules](#rules)
  - [Motion and encoder](#motion-and-encoder)
  - [Line and barcode](#line-and-barcode)
  - [Obstacle](#obstacle)
  - [Terrain](#terrain)
  - [Mission and Safety](#mission-and-safety)
  - [Comms](#comms)
  - [Queues, rings and messages](#queues-rings-and-messages)
  - [MQTT frames](#mqtt-frames)
  - [Fault bitmask](#fault-bitmask)
- [7. Correctness and performance evidence](#7-correctness-and-performance-evidence)
  - [No lost function](#no-lost-function)
  - [Memory](#memory)
  - [Timing](#timing)

Single owner of: members, subsystem ownership, pins/wiring, current kernel-object inventory, coding rules, the data representation table and its evidence. Architecture owns target scheduling and cross-subsystem contracts. Build, flash and bring-up: [README](../README.md#getting-started). Requirements and pending decisions: [project-specs](project-specs.md). Shared-data rules and MQTT frame: [architecture](architecture.md).

A **subsystem** is one student's folder (may hold several tasks and peripherals); a **task** is a thread of execution; a **peripheral** is a hardware block. Subsystem owners retain responsibility for their folders; coordinate firmware changes with the owner. This documentation reconciliation is authorized by Nicholas. All members join integration, debugging, system testing and demonstration.

## 1. Members and subsystems

| # | Subsystem | Student ID | GitHub | Code | Tests | Guide |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | **Shaun, Communications:** WiFi/MQTT, telemetry, reporting | 2500629 | shauniee | [app_program/comms/](../app_program/comms/) | [testing/comms/](../testing/comms/) | [comms](../app_program/comms/README.md) |
| 2 | **Mahesha, Motion:** motors, encoders, wheel PID, motion APIs | 2503973 | itsjustmdawg | [app_program/motion/](../app_program/motion/) | [testing/motion/](../testing/motion/) | [motion](../app_program/motion/README.md) |
| 3 | **Nicholas, Line, Barcode, Mission integration:** IR, barcode, coordination, integration | 2501552 | nicholas-bay | [app_program/line_barcode/](../app_program/line_barcode/) | [testing/line_barcode/](../testing/line_barcode/) | [line_barcode](../app_program/line_barcode/README.md) |
| 4 | **Darrius, Terrain:** IMU processing, motion classification, hump measurement | 2500360 | Galaxius03 | [app_program/terrain/](../app_program/terrain/) | [testing/terrain/](../testing/terrain/) | [terrain](../app_program/terrain/README.md) |
| 5 | **Jia Jing, Obstacle:** sonar, servo scanning, obstacle profiling | 2502825 | JJLOLLOL | [app_program/obstacle/](../app_program/obstacle/) | [testing/obstacle/](../testing/obstacle/) | [obstacle](../app_program/obstacle/README.md) |

Stable test IDs/measurements: [testing](../testing/README.md). Demonstrations: [video](../video/README.md). Course/calibration decisions are recorded once in [project-specs](project-specs.md#pending-decisions).

**Implementation status.** The current source baseline and completion
plan are linked from [pending work](../pending.md). The code
contains Motion PID/actions, Mission/Safety, line/barcode, Terrain, Obstacle and
Reporting modules; this is not physical acceptance. Older smaller bench/task
arrangements are historical. Target coordination is in [architecture](architecture.md).
Nicholas is the sole contact for pending decisions and approvals; historical subsystem attribution and code writer responsibilities remain.

## 2. Pins

| Pin | Owner | Used for |
| --- | --- | --- |
| GP23, GP24, GP25, GP29 | Kernel (reserved) | Pico W radio/internal signals; never allocate externally |
| GP0 | Mahesha | Right encoder B, Grove 1, green; UART0 console/crash conflict |
| GP1 | Mahesha | Right encoder A, Grove 1, red; UART0 console/crash conflict |
| GP2 | Jia Jing | Reported HC-SR04P Trig, Grove 2; shares Maker port |
| GP3 | Jia Jing | Reported HC-SR04P Echo, Grove 2; shares Maker port |
| GP4 | Darrius | GY-511 SDA, Grove 3 / I2C0 |
| GP5 | Darrius | GY-511 SCL, Grove 3 / I2C0 |
| GP6, GP13, GP14, GP15, GP19 | Unclaimed | Check carrier sharing before allocation |
| GP7 | Mahesha | Left encoder B, Grove 7, green |
| GP8 / GP9 | Mahesha | Left motor M1A (black) / M1B (yellow); exclusive motor writer |
| GP10 / GP11 | Mahesha | Right motor M2A (black) / M2B (yellow); exclusive motor writer |
| GP12 | Jia Jing | SG90 Servo 1 signal S, orange; bench holds signal low |
| GP16 | Unclaimed | Grove 4 first signal; barcode IR DOUT until 2026-11-09 |
| GP17 | Nicholas | Right line IR DOUT, digital input (Grove 4 second signal on the carrier); AOUT not connected |
| GP18 | Unclaimed | Onboard RGB LEDs; no firmware owner |
| GP20 | Nicholas | Onboard active-low calibration/start; target fresh start only after eligible reset |
| GP21 | Nicholas | Onboard active-low stop/cancel; target eligible release-to-ready handshake |
| GP22 | Nicholas | Onboard buzzer; not used by the firmware |
| GP26 | Nicholas | Left line IR DOUT, Grove 5 / digital input, ADC not used; AOUT not connected; Grove 6 shared GP26 contact disconnected |
| GP27 | Nicholas | Barcode IR DOUT, digital input with edge interrupt, ADC not used; AOUT not connected |
| GP28 | Mahesha | Left encoder A, Grove 7, red; ADC2-capable but used digitally; open battery jumper |

User confirmation 2026-10-29: physical wiring matches this allocation and sonar electrical behavior is confirmed; motor/encoder polarity is not confirmed. Nicholas, 2026-11-05: wiring checked and unchanged, and no AOUT wire is connected on any of the three IR sensors. This does not append a measured test result. Nicholas, 2026-11-09: rewired to left line DOUT GP26, barcode DOUT GP27 and right line DOUT GP17; which connector each lead uses is not recorded.

### Supply and wires

- Power bank: red to green terminal VIN+, black to VIN-. Verify polarity.
- Encoders: white to 3V3, blue to GND. Line/barcode IR: VCC to 3V3, GND to GND; connect left DOUT to GP26, right DOUT to GP17 and barcode DOUT to GP27; all three AOUT pins stay unconnected.
- GY-511: Vin to 3V3, GND to GND; separate `3.3`, INT1, INT2, DRDY disconnected. Reported HC-SR04P: VCC to 3V3, GND to GND, Trig to GP2, Echo to GP3; Echo HIGH is 3.3 V (Nicholas, 2026-11-05, not measured).
- SG90: red to +, brown to -; verify carrier silkscreen and supply rail.

### Wiring conflicts

- Grove 5 and Grove 6 share GP26; joining independent sensor outputs is unsafe, keep the Grove 6 GP26 contact disconnected.
- GP28 battery sensing stays disconnected via its jumper while GP28 is the left encoder.
- UART0 console/crash reporting claim GP0/GP1: use `CONSOLE=usb_cdc CRASH_REPORT=0` with encoders attached.
- Grove 2 and the Maker port share GP2/GP3; do not attach independent devices to both.
- Disconnect GY-511 wires reused elsewhere before restoring I2C; its separate `3.3` pin is not established as an extra supply input.
- All three IR sensors are read digitally on DOUT, so no ADC is used for sensing. GP17, GP26, GP27 and GP28 are taken as digital inputs, so connecting any AOUT needs a reviewed allocation change or extra hardware.

### Physical header map (W/WH; header numbers differ from GPIO numbers)

| Pin | Signal | Pin | Signal | Pin | Signal | Pin | Signal |
| ---: | --- | ---: | --- | ---: | --- | ---: | --- |
| 1 | GP0 | 11 | GP8 | 21 | GP16 | 31 | GP26 / ADC0 |
| 2 | GP1 | 12 | GP9 | 22 | GP17 | 32 | GP27 / ADC1 |
| 3 | GND | 13 | GND | 23 | GND | 33 | AGND |
| 4 | GP2 | 14 | GP10 | 24 | GP18 | 34 | GP28 / ADC2 |
| 5 | GP3 | 15 | GP11 | 25 | GP19 | 35 | ADC_VREF |
| 6 | GP4 | 16 | GP12 | 26 | GP20 | 36 | 3V3 OUT |
| 7 | GP5 | 17 | GP13 | 27 | GP21 | 37 | 3V3_EN |
| 8 | GND | 18 | GND | 28 | GND | 38 | GND |
| 9 | GP6 | 19 | GP14 | 29 | GP22 | 39 | VSYS |
| 10 | GP7 | 20 | GP15 | 30 | RUN | 40 | VBUS |

## 3. Task priorities and kernel object budget

[Architecture](architecture.md#tasks) owns the only target task/core/scheduling
table. [robot_config.h](../app_program/common/robot_config.h) contains current
provisional task settings. Keep Motion/Mission on core 1 and I/O on core 0;
no core swap. Existing priorities/periods are retained provisionally with user authorization;
D13 physical scheduling acceptance remains pending.
`TP_PRC1` means core 0 and `TP_PRC2` core 1; lower priority number runs first.

Working software allocation inventory after #130–#133 (excluding kernel/driver
allocations; not measured live high-water marks):

| Application object | Allocation / ownership |
| --- | --- |
| Tasks | Motion and Mission on core 1, I/O on core 0: exactly three creation call sites |
| Mutexes | Four: Motion command/health, line snapshot, Obstacle profile, cross-core events |
| Event flags | One persistent Motion stop/inhibit flag |
| Static rings | Existing barcode capture/events and Comms bridge/events; no generic bus |
| Task stacks | 2048 bytes requested each, 6144 bytes total requested; trace images report use ([timing](#timing)) |

Limits in [config/config.h](../config/config.h) are total capacities, including
kernel/driver objects. Recount actual allocations and measure memory after task
consolidation; no invented savings or margin. Required control initialization
failure inhibits startup. Optional data failure is handled by the central policy;
normal mission readiness still requires operational WiFi and MQTT.

## 4. Interfaces between subsystems

Each provider owns private storage and exposes coherent accessors. Current APIs
are inventoried in subsystem guides; target authority/sharing/freshness is owned
by [architecture](architecture.md#shared-data-rules).

| Path | Ownership / target rule |
| --- | --- |
| Mission to Motion | One latest packed command, bounded mutex copy; Motion owns feedback and stop latch |
| Local GP21 to Motion | Independent input check, persistent stop; no command-mutex/telemetry dependency |
| line_barcode evidence to Mission | Bounded in-task sensing/decoder steps; retain original acquisition times |
| Mission to Obstacle | Scan/side-watch intent and bounded acquisition steps, never a second motor writer |
| Terrain to Mission/Motion | Core-0 coherent evidence with age/validity; no cross-core runtime `motion_stop()` call |
| I/O to Mission mapping | Bounded hand-off; Mission installs whole mapping while stopped and returns result before ACK |
| Mission/Terrain to Reporting | Cross-core event synchronization required; same-core DI/EI is insufficient |
| Reporting to kernel MQTT | Opaque bounded TX/RX bridge; only WiFi context invokes networking APIs |

Record changed interface layouts before coding and append integration results in
[testing/integration/TEST_LOG.md](../testing/integration/TEST_LOG.md). A valid
software copy or brake acknowledgement is not physical stationarity.

## 5. Coding rules

Apply to new or changed application code (imported kernel need not be reformatted). The standard is BARR-C:2018 (Barr Group), applied as the source allows for legacy code: changed lines must comply, and older code is converted as it is touched. Its license limits it to internal use, so keep it out of this repository. This is not a compliance certification. `python3 scripts/check_barr.py` scans changed `app_program` lines for the mechanical rules below (`--all` for every line); a Claude Code hook runs it once Claude has finished its edits, before it reports them. The review checklist stays manual.

- **Language/layout:** GNU C11 (`-std=gnu11`). Use C11 acquire/release for word snapshots and startup publication; no atomic RMW helpers on Cortex-M0+. Isolate other extensions in drivers. Four spaces, LF, one statement per line, braces on every body, opening brace on its own line, ~80 columns. Space operators, parenthesize precedence, comment purpose/units/bounds/assumptions, remove dead code.
- **Modules:** one purpose each; lowercase `snake_case.c/.h`, guarded header, minimal exports; never include a `.c` or absolute path. Public functions prefixed with module name; private `static`. Named constants; prefer functions to macros (macros evaluate args once, parenthesized). Types `module_name_t`.
- **Data:** fixed-width ints where width matters; packed bits for stored/shared flags; `bool` only for locals/returns; `const` for immutable. Consistent unsigned literals; justify narrowing casts; avoid signed bit ops. Initialize before use; explicit invalid codes. `float` only for justified local computation (`f` suffix, finite/range checks); stored/shared values are fixed point with documented scale (widths and scales in [section 6](#6-data-representation)). In-RAM packed state may use bit-fields; wire formats (MQTT) use explicit masks/shifts with documented byte order, never native structs, padding or bit-field order.
- **Minimum-bit:** each field gets `ceil(log2(values))` bits including the invalid code; record range, unit, signedness, scale, rounding, invalid code, overflow behavior in [section 6](#6-data-representation) before converting. Exclusive states use one encoded value; coexisting faults use independent bits. Combine fields in one unit only with a shared writer. Access via `module_field_get()/_set()`; no caller masks. Loop counters/temporaries exempt. Test min, max and invalid code; overflow saturates or faults as documented.
- **Control:** check return codes; safe failure responses; `default` in switches; document fallthrough. Bound polling, waits, retries, buffers, actions; validate counts/indices/inputs; keep acquisition time; wrap-safe elapsed time.
- **Concurrency:** `volatile` is for hardware/async access, not synchronization. Share data only via the three patterns in [architecture](architecture.md): one-word snapshot (single store/load), mutex copy above 32 bits, ISR-to-task ring. One writer per storage unit; only it does RMW. Packed data shared with an ISR on the same core updates inside `DI`/`EI`; packed data written from both cores needs a mutex or RP2040 hardware spinlock (`DI` masks only the local core). ISRs: capture, signal, return; no blocking, formatting, decoding, allocation; use `_task`/`_isr` names.
- **Ownership:** one motor writer, separate peripheral owners, kernel-only networking; Stop must not depend on diagnostics queues or the request mutex. No credentials in version control.

**Review checklist:** purpose/API/units/ownership clear, no duplicate mutable state; style consistent; inputs, casts, arithmetic, finite values, bounds checked; hardware actions bounded, failures inhibit unsafe output; shared values synchronized, cached observations keep acquisition time; stop, stale data and reset keep the contract; no ISR blocking, foreign motor writes, student network calls or committed credentials; packed fields match [section 6](#6-data-representation), use accessors, have boundary tests; one writer per storage unit; evidence names source/profile (a build is not hardware proof).

## 6. Data representation

This table owns the width of every application data field. The requirement it
answers is in [project-specs section 4](project-specs.md#4-change-in-requirement-minimum-bit-data-representation);
MQTT byte layouts are in [architecture](architecture.md#mqtt-binary-protocol-schema-version-3).
Bits are written as **logical / stored** when they differ: logical is what the
range needs, stored is what the field occupies.

### Rules

- **Bits** = `ceil(log2(valid values + invalid codes))`.
- **Flags** are `bool name : 1`. Assignment normalises any non-zero value to 1,
  so a flag can never be truncated to the wrong value.
- **Codes and ranges** are unsigned bit-fields (`uint8_t name : N`). A
  `_Static_assert` beside the field proves the largest enumerator or configured
  maximum fits, so a later config change fails the build instead of truncating.
  Computed values are clamped or saturated before the store.
- **Signed values** use the smallest signed type (`int8_t`, `int16_t`).
  BARR-C:2018 rule 5.3.a forbids new signed bit-fields. Signed bit-fields that
  existed before the change (profile coordinates, bypass Q8 pose, Motion
  target and acceleration, turn length, calibration sums) are kept.
- **Shared data** has one writer per storage unit. A single value shared across
  cores or with an ISR uses the smallest lock-free C11 atomic (8, 16 or 32-bit
  load and store; Cortex-M0+ has no atomic read-modify-write). Atomics cannot be
  bit-fields, so their storage can exceed the logical bits; spare bits are zero.
  Several fields in one atomic word use `rd_field_get`/`rd_field_set` masks.
  Anything over 32 bits is copied under a mutex.
- **Times** stay `uint32_t` (ms or us) so elapsed-time arithmetic is wrap-safe;
  16-bit milliseconds would wrap every 65.5 s, inside one run. **Change
  counters** (`sequence`) stay 32-bit and are only compared for equality.
- **Floats** are per-release computation only. The one stored float, the
  terrain trace travel, is now Q8 fixed point.
- **Wire frames** use explicit masks and shifts, little-endian, never a struct
  copy. Loop counters and one-function locals are exempt.

### Motion and encoder

| Field | Where | Valid range | Bits | Representation | Writer → readers, sync |
| --- | --- | --- | ---: | --- | --- |
| Encoder count, per wheel | `motion.c` `counts[2]` | 0..65535, wraps (`ENC_COUNT_BITS`) | 16 | `_Atomic enc_count_t` | encoder ISR (core 1) → all tasks; release/acquire |
| Quadrature level, per wheel | `motion.c` `previous_levels` | 0..3 | 2+2 / 8 | one byte, masks per wheel | encoder ISR only |
| Invalid edges | `motion.c` `invalid_edges` | change counter | 32 | `_Atomic uint32_t` | ISR → Motion |
| Command target | `MotionCommand.target` | -2000..2000 mm or -360..360 deg | 12 / 16 | `int16_t` | Mission → Motion, mutex copy |
| Command speed | `speed_action` bits 0-8 | 0..500 mm/s | 9 | mask and shift | same |
| Command action ID | `speed_action` bits 9-14 | 0..63 | 6 | mask and shift; bit 15 spare | same |
| Command mode | `mode_steering` bits 0-1 | STOP, FOLLOW, DISTANCE, TURN | 2 | mask and shift | same |
| Command steering | `mode_steering` bits 2-9 | -127..127, Q7 | 8 | mask and shift; bits 10-15 spare | same |
| Command time | `MotionCommand.sampled_ms` | ms, wraps | 32 | `uint32_t` | same |
| Feedback | `motion.c` `feedback` | status 0..3, action 0..63 | 8 / 32 | `rd_word_t`, bits 0-1 and 2-7 | Motion → Mission, Comms; one word |
| Health flags | `MotionHealth.flags` | stopped, braked, serviced once | 3 / 8 | bits 0-2 | Motion → Mission, Safety, Terrain; mutex copy |
| Health time | `MotionHealth.serviced_ms` | ms | 32 | `uint32_t` | same |
| Stop requests, stops serviced | `motion.c` | change counters | 32 each | `_Atomic uint32_t`; core 0 loads serviced first | core-1 callers under DI, Motion → Terrain (core 0) |
| Action target | `MotionController` | -2000..2000 | 12 | signed bit-field (kept) | Motion only |
| Turn style, turn arrived | `MotionController` | 0..3 each | 2 each | bit-fields | Motion only |
| Mode, status | `MotionController` | 0..3 each | 2 each | bit-fields | Motion only |
| Active, action ID | `MotionController` | 0..1, 0..63 | 1, 6 | bit-fields | Motion only |
| Follow base speed | `MotionController` | Q6 mm/s | 15 | bit-field | Motion only |
| Profile acceleration | `MotionController` | signed mm/s² | 11 | signed bit-field (kept) | Motion only |
| Origin and previous counts | `MotionController` | encoder counts | 16 each | `enc_count_t` | Motion only |
| PID integral, previous speed | `WheelPid` | Q4 counts, counts/s | 16 each | `int16_t` | Motion only |
| Input counts | `MotionInput.count[2]` | encoder counts | 16 each | `enc_count_t` | per release |
| Output status, action, stall | `MotionOutput` | 0..3, 0..63, flag | 2, 6, 1 | bit-fields | per release |

### Line and barcode

| Field | Where | Valid range | Bits | Representation | Writer → readers, sync |
| --- | --- | --- | ---: | --- | --- |
| IR level, per sensor | `IrRaw` | 0..1; 2..3 driver error | 2 each | bit-fields | per acquisition |
| IR raw word | `IrRawWord` | left, right, valid | 3 / 8 | `uint8_t` bits 0-2, one byte store | Mission task |
| IR fraction, per sensor | `IrReading` | 0 or 127 (`IR_SCALE_ONE`) | 7 each | bit-fields | per acquisition |
| IR error | `IrReading`, `IrTrackingOutput` | -127..127 | 8 | `int8_t` | per acquisition |
| Line status | `IrTracker`, `LineIrSnapshot`, `IrTrackingOutput` | unknown, visible, lost, broad black | 2 | bit-field | Mission task |
| Sample flags | `LineIrSnapshot` | black L/R, have sample, fresh, valid, guidance | 1 each | bit-fields | Mission → Mission, Safety, Comms; mutex copy |
| Sample time | `IrTracker`, `LineIrSnapshot` | ms | 32 | `uint32_t` | same |
| Edge ring entry | `barcode.c` `ring[64]` | two counts and a level | 33 | two `uint16_t` plus one bit of `ring_high` | GP27 ISR → Mission (same core); SPSC atomics |
| Edge counts, level | `barcode_edge_t` | counts, 0..1 | 16 each, 1 | `barcode_count_t`, bit-field | per pop |
| Element widths | `barcode_frame_t.widths` | 16..400 counts, 25 elements | 9 each | 29 packed bytes, masks behind `barcode_frame_width()` | Mission task |
| Chatter carry | `barcode_frame_t.carry` | 0..415 counts | 9 | bit-field | Mission task |
| Element count | `barcode_frame_t.count` | 0..25 | 5 | bit-field | Mission task |
| Last width | `barcode_frame_t.last_width` | 0..32767 counts, diagnostics | 15 | bit-field | Mission task |
| Have edge, previous level | `barcode_frame_t` | flags | 1 each | bit-fields | Mission task |
| Decoded event | `barcode_event_t.packed` | letter 0..25 (31 none), action 0..4, orientation | 5+3+1 / 16 | masks behind accessors | Mission task queue, 8 entries |
| Mapping | `barcode.c` `mapping_word` | four letters 0..25, mapped flag | 21 / 32 | `rd_word_t`, bits 0-19 and 20 | Mission → Mission, Comms; one word |
| Scanner claimed, running | `barcode.c` `scanner` | flags | 1 each | bit-fields | Mission task |
| Edges seen | `barcode.c` `edge_total` | change counter, read as 5 bits | 32 | `_Atomic uint32_t` | ISR → Mission diagnostics |

### Obstacle

| Field | Where | Valid range | Bits | Representation | Writer → readers, sync |
| --- | --- | --- | ---: | --- | --- |
| Range word | `obstacle_range_sample_t.word` | 0..8190 mm, 8191 invalid; valid flag | 14 / 16 | `uint16_t` bits 0-12 and 13 | Obstacle (Mission task) → Mission; mutex mirror → Comms |
| Sample wheel counts | `obstacle_range_sample_t` | encoder counts | 16 each | `uint16_t` | same |
| Sample time, sequence | `obstacle_range_sample_t` | ms; change counter | 32 each | `uint32_t` | same |
| Echo state | `obstacle.c` `echo_state` | idle, armed, high, done | 2 / 8 | `_Atomic uint8_t` | Echo ISR ↔ Mission (same core) |
| Echo pulse width | `obstacle.c` `pulse_us` | 0..30000 us, saturates | 15 / 16 | `_Atomic uint16_t` | Echo ISR → Mission |
| Consecutive misses | `obstacle_acquisition_t` | 0..3, saturates at the limit | 2 | bit-field | Mission task |
| Ray | `ObstacleRay` | angle ±60 deg, 0..8190 mm, valid | 8+13+1 | packed 3 bytes | Mission task |
| Profile distances | `ObstacleProfile` | closest, width, clearances 0..1023 mm | 10 each | bit-fields, saturate | Mission → Comms; mutex copy |
| Profile coordinates | `ObstacleProfile` | ±1023 mm; angle ±60 deg | 11 each; 7 | signed bit-fields (kept) | same |
| Width uncertainty | `obstacle_profile_evidence_t` | 0..1023 mm, 2047 unknown | 11 | bit-field | same |
| Planning width | `obstacle_profile_evidence_t` | 0..1023 mm | 10 | bit-field | same |
| Ray counts, edge flags | `obstacle_profile_evidence_t` | 0..15; flags | 4 each; 1 each | bit-fields | same |
| Range group | `obstacle_range_group_t` | five ranges; counts 0..5 | 13 / 16 each; 3 each | `uint16_t` array; bit-fields | Mission task |
| Scan angles | `obstacle_acquisition_t.angles[15]` | ±60 deg | 7 / 8 each | `int8_t` | Mission task |
| Scan bookkeeping | `obstacle_acquisition_t` | counts 0..15, id 0..15, phase 0..2, flags | 4, 4, 2, 1 | bit-fields | Mission task |
| Scan word | `obstacle.c` `scan_word` | state 0..7, scan id 0..15 | 7 / 32 | `rd_word_t` bits 0-2, 3-6 | Mission task → Mission, Comms |
| Side watch | `obstacle.c` `side_command` | on, angle ±60 deg | 9 / 32 | `rd_word_t` bit 0, bits 1-8 | Mission task |
| Requested scan id | `obstacle.c` | 0..15 | 4 / 8 | `_Atomic uint8_t` | Mission task |

### Terrain

| Field | Where | Valid range | Bits | Representation | Writer → readers, sync |
| --- | --- | --- | ---: | --- | --- |
| Terrain word | `terrain.c` `terrain_word` | last and max code 0..127, count 0..255, state 0..2, height valid, calibrated | 7+7+8+2+1+1 = 26 / 32 | `rd_word_t`; bit 24 and 27-31 spare | I/O (core 0) → Comms |
| Startup outcome | `terrain.c` | pending, done, failed | 2 / 8 | `_Atomic uint8_t` | I/O (core 0) → Mission (core 1) |
| Filtered acceleration | `TerrainContext` | Q16 raw LSB | 32 each | `int32_t` | I/O only |
| Calibration sums | `TerrainCalibSum` | ±3276800 | 23 each | signed bit-fields, packed 9 bytes (kept) | I/O only |
| Roll baseline | `TerrainContext.roll_base_q8` | ±2.3 M, Q8 cdeg | 23 / 32 | `int32_t` (signed) | I/O only |
| Peak rise | `TerrainContext.rise_q` | 0..1200, 0.1 mm | 11 | bit-field, saturates | I/O only |
| Tilted travel | `TerrainContext.travel_counts` | 0..4660 counts | 13 | bit-field, saturates | I/O only |
| Pitch, roll | `TerrainContext` | ±9000 cdeg | 15 / 16 each | `int16_t` | I/O only |
| Hump codes, count | `TerrainContext` | 0..127; 0..255 | 7 each; 8 | bit-fields; `uint8_t` | I/O only |
| Flat run, calibration count | `TerrainContext` | 0..10; 0..100 | 4; 7 | bit-fields | I/O only |
| State, height valid, calibrated | `TerrainContext` | 0..2; flags | 2; 1 each | bit-fields | I/O only |
| Trace travel | `terrain.c` | mm, wraps at 65536 | 32 | `uint32_t` Q8; wire takes bits 8-23 | I/O only |

### Mission and Safety

| Field | Where | Valid range | Bits | Representation | Writer → readers, sync |
| --- | --- | --- | ---: | --- | --- |
| Published state | `mission.c` | 7 states | 3 / 8 | `_Atomic uint8_t` | Mission → Comms (core 0) |
| State, phase | `Mission` | 0..6; 0..4 | 3 each | bit-fields | Mission only |
| Held barcode action | `Mission.held` | 0..4 | 3 | bit-field | Mission only |
| Turn code, onset black, front clear | `Mission` | 0..3 each | 2 each | bit-fields | Mission only |
| Scan id | `Mission` | 0..15 | 4 | bit-field | Mission only |
| Junction and scan flags (12) | `Mission` | flags | 1 each | bit-fields | Mission only |
| Line integral | `Mission.line_i_q` | 0..45, Q7 | 6 | bit-field | Mission only |
| Action IDs | `Mission.action_id`, `current_action` | 1..63 | 6 each | bit-fields | Mission only |
| Turn length | `Mission.turn_length_mm` | ±2000 mm | 12 | signed bit-field (kept) | Mission only |
| Touch sides | `Mission.line_side`, `line_prev`, `cross_side` | -1, 0, +1 | 2 / 8 each | `int8_t` (signed) | Mission only |
| Steering target | `Mission.line_steer_q` | -127..127, Q7 | 8 | `int8_t` | Mission only |
| Reverse distances | `Mission.backup_mm`, `backed_mm` | 0..150 mm | 8 each | `uint8_t` | Mission only |
| Origins | `Mission` | encoder counts | 16 each | `enc_count_t` | Mission only |
| Ranges in | `MissionInputs` | front and side 0..8190 mm; clearances, profile distances 0..1023 | 13; 10 | bit-fields | per release |
| Codes in | `MissionInputs` | line 0..3, motion status 0..3, action 0..63, scan state 0..7, scan id 0..15, letter 0..31, edges 0..31, barcode action 0..4 | 2, 2, 6, 3, 4, 5, 5, 3 | bit-fields | per release |
| Flags in (17) | `MissionInputs` | flags | 1 each | bit-fields | per release |
| Counts in | `MissionInputs` | encoder counts | 16 each | `enc_count_t` | per release |
| Event out | `MissionOutputs` | kind 0..4, arg 0..127, arg2 0..1023 | 3, 7, 10 | bit-fields | per release |
| Scan id, rejection out | `MissionOutputs` | 0..15; 0..3 | 4; 2 | bit-fields | per release |
| Side watch angle out | `MissionOutputs` | ±60 deg | 7 / 8 | `int8_t` | per release |
| Flags out (9) | `MissionOutputs` | flags | 1 each | bit-fields | per release |
| Bypass route, work | `bypass_control_t` | 0..8; 0..3 | 4; 2 | bit-fields | Mission only |
| Bypass pose | `bypass_control_t` | Q8 mm | 21 each | signed bit-fields (kept) | Mission only |
| Bypass edge budget | `bypass_control_t.edge_counts` | 0..93192 counts | 17 | bit-field | Mission only |
| Bypass half width | `bypass_control_t.half_width_mm` | 0..511 mm | 9 | bit-field | Mission only |
| Bypass samples, flags, action ID | `bypass_control_t` | 0..7, 0..3, flags, 0..63 | 3, 2, 1, 6 | bit-fields | Mission only |
| Bypass return saw white | `bypass_control_t.return_white` | flag | 1 | bit-field | Mission only |
| Bypass geometry | `bypass_control_t` | centre, front ±1023 mm; offset, flank, heading, target unbounded | 11 / 16; 16 | `int16_t`, `uint16_t` | Mission only |
| Bypass and recovery inputs | `bypass_input_t`, `recovery_input_t` | counts; ranges 0..8190; status 0..3; action 0..63; flags | 16; 13; 2; 6; 1 | `enc_count_t`, bit-fields | per release |
| Recovery phase | `recovery_control_t`, `recovery_output_t` | 0..3 | 2 | bit-field | Mission only |
| Recovery turn | `recovery_control_t` | ±360 deg | 10 | signed bit-field (kept) | Mission only |
| Latched causes | `safety.c` `latched` | 4 cause bits | 4 / 32 | `rd_word_t` | Mission (Safety step) → Mission |
| Reset state | `safety.c` | 0..3 | 2 / 8 | `uint8_t`, passed by pointer | Mission only |
| Button flags | `Button`, `ButtonInputs` | flags | 1 each | bit-fields | Mission task |
| Safety inputs | `SafetyInputs` | flags; ages in ms | 1 each; 32 | bit-fields; `uint32_t` | per release |
| Mapping request | `mission_mapping.c` `request_word` | letters 20, mapped 1, sequence 8, direction 3 | 32 | `rd_word_t` bits 0-31 | I/O (core 0) → Mission |
| Mapping reply | `mission_mapping.c` `reply_word` | sequence 8, result 2 | 10 / 32 | `rd_word_t` | Mission → I/O |

### Comms

| Field | Where | Valid range | Bits | Representation | Writer → readers, sync |
| --- | --- | --- | ---: | --- | --- |
| Last barcode | `comms.c` | letter 0..25 (31 none, also after its navigation), action 0..4 | 5+3 = 8 | `_Atomic uint8_t` bits 0-4, 5-7 | Mission → Comms (core 0) |
| Last clearance | `comms.c` | 0..1023 mm | 10 / 16 | `_Atomic uint16_t`, saturates | Mission → Comms |
| Link state | `report.c` | Wi-Fi up, broker up | 2 / 8 | `_Atomic uint8_t` | transport → I/O (core 0) |
| Event entry | `comms_packed_event_t` | kind 1..4, arg 0..127, time, counts, arg2 0..1023 | 3+7 / 16, 16, 16+16, 10 / 16 | masks behind accessors | Mission, Terrain → Comms; cross-core mutex |
| Event ring indices | `comms_event_log_t.indices` | head 0..15, count 0..16 | 4+5 / 16 | masks | same |
| Bridge slot | `slot_t` | length 1..18, frame bytes | 5, 18 x 8 | bit-field, byte array | I/O ↔ transport (core 0); SPSC atomics |

### Queues, rings and messages

There are no kernel message buffers or mailboxes. Data moves through these
fixed rings and copies.

| Buffer | Entries | Entry | Sync |
| --- | ---: | --- | --- |
| Barcode edge ring | 64 | 33 bits (two counts, one level bit) | ISR → Mission, SPSC atomics, same core |
| Barcode event queue | 8 | `barcode_event_t`, 6 bytes | Mission task only |
| Barcode, wheel, line trace rings | 8 each | encoded 18, 15 and 17-byte frames | producer → I/O, SPSC atomics (trace images only) |
| Comms event ring | 16 | 10 bytes; 162 bytes in all | cross-core mutex, bounded wait |
| Bridge out / in | 4 / 2 | 19-byte slot | SPSC atomics, core 0 |
| Snapshots | 1 each | `MotionCommand` 12, `MotionHealth` 8, `LineIrSnapshot` 8, profile 20, range samples 2 x 16 bytes | mutex copy |

### MQTT frames

Every frame is compact binary with explicit masks and shifts; no value is sent
as text. Byte layouts are in [architecture](architecture.md#mqtt-binary-protocol-schema-version-3).
The only text is the retained status topic `"1"`/`"0"`, set by the kernel's
MQTT last-will, outside the application payloads.

| Frame | Bytes | Used / stored bits |
| --- | ---: | --- |
| Telemetry and snapshot reply | 18 | 135 / 144 |
| Event | 11 | 83 / 88 |
| Mapping | 5 | 36 / 40 |
| Acknowledgement | 3 | 18 / 24 |
| Snapshot request | 2 | 16 / 16 |
| Junction test | 3 | 19 / 24 |
| Wheel trace | 15 | 120 / 120 |
| Line trace | 17 | 135 / 136 |
| Barcode trace | 18 | 144 / 144 |
| Terrain trace | 14 | 109 / 112 |

Telemetry carries the twelve defined fault bits as one 12-bit fault word
(schema v3; 4 bits reserved). In RAM each owner keeps its own atomic byte,
because each owner is the only writer of its byte and Cortex-M0+ has no atomic
read-modify-write for a word shared by writers on both cores.

### Fault bitmask

One bit per condition, one `_Atomic uint8_t` per owner, written only by that
owner. Simultaneous faults stay independent. On the wire the defined bits go
into the 12-bit fault word in this order: Motion bits 0-1, Line 2-4, Terrain
5-7, Obstacle 8, Mission 9, Network 10-11.

| Owner (writer) | Bit 0 | Bit 1 | Bit 2 |
| --- | --- | --- | --- |
| Motion (Motion) | motor stall | encoder | - |
| Line (Mission task) | IR invalid | barcode capture overflow | barcode event overflow |
| Terrain (I/O) | IMU | calibration failed | impact |
| Obstacle (Mission task) | no echo | - | - |
| Mission (Mission) | line lost | - | - |
| Network (kernel transport) | Wi-Fi lost | broker lost | - |

## 7. Correctness and performance evidence

### No lost function

- **Compile-time proof.** Every narrowed field has a `_Static_assert` that its
  largest enumerator or configured maximum fits. Changing a config value past a
  field's width fails the build.
- **Boundary tests.** `sh testing/pure/run.sh` runs the host suite against the
  real `app_program` sources. Tests that cover packed data include
  `test_mqtt_frame.c` (golden vectors, every maximum, one past maximum and
  reserved bits), `test_robot_data.c` (field accessors, torn-read stress),
  `test_barcode.c` (`test_packed_widths`: the 9-bit window keeps the newest 25
  widths exactly, minimum and maximum included), `test_echo_range.c` (range
  word spare bits, miss count saturating at its 2-bit limit), `test_terrain.c`
  (2-bit state masking, spare bit zero) and `test_comms_stress.c` (terrain word
  under two-thread load).
- **Wire format.** Schema v3 changes only the fault field (six bytes to one
  12-bit word) and the version nibble; every other field keeps its width and
  meaning. The C codec and `scripts/mqtt_client.py --self-test` decode the same
  new golden vectors, and S1-U14 walks each of the 12 fault bits alone and all
  together.
- **Builds.** All six profiles in `testing/build_pending_profiles.sh` compile
  with `-Wall -Wextra`, and the Arm size probes pass their size bounds.
- **Physical.** A course trial on the final image is still to be run (INT-225).

### Memory

Target `sizeof`, Arm GNU C, RP2040, measured with
`sh testing/sizeof_probe.sh`
on `main` (`c26bca4`) and on this change (INT-223).

| Type | Before (bytes) | After (bytes) |
| --- | ---: | ---: |
| `MissionInputs` | 104 | 64 |
| `barcode_frame_t` | 62 | 38 |
| `bypass_input_t` | 60 | 40 |
| `Mission` | 160 | 140 |
| `MissionOutputs` | 32 | 20 |
| `recovery_input_t` | 24 | 16 |
| `bypass_control_t` | 64 | 56 |
| `comms_event_log_t` | 168 | 162 |
| `obstacle_range_sample_t` | 20 | 16 |
| `bypass_output_t`, `SafetyInputs` | 20 each | 16 each |
| `MotionInput`, `MotionHealth`, `LineIrSnapshot` | 12 each | 8 each |
| `MotionController` | 48 | 44 |
| `IrRaw` | 4 | 1 |

| Image | Text before → after | Static RAM (bss) before → after |
| --- | --- | --- |
| Course (`TRACE=0`) | 387,912 → 390,356 B (+2,444) | 83,836 → 83,728 B (-108) |
| Trial (`TRACE=1`) | 389,712 → 393,208 B (+3,496) | 84,284 → 90,724 B (+6,440) |

Course static RAM falls by 108 bytes, and by 28 more with the schema v3 fault word (bridge slots and the receive buffer shrink from 22 to 18 bytes: course bss 83,700 B, text 390,420 B). Most of the saving is on the Mission and
Motion stacks, which hold the per-release input and output copies. Flash grows
by about 2.4 KB, the cost of reading and writing bit-fields: 0.6 percent of the
image and well inside the 2 MB part. The trial image's extra RAM is the timing
instrumentation's static task stacks (3 x 2,176 bytes), which the kernel then
no longer allocates from its own heap; the course image has no instrumentation.

### Timing

Trace images (`./flash.sh trial` and `./flash.sh test`) print one line every
5 s on the USB console:

```text
[TIMING] t=<ms> | motion max=<us>us ovr=<n> stack=<used>/<size>B | io ... | mission ...
```

- `max` is the longest task body in the last 5 s, measured with the 1 MHz timer.
- `ovr` counts releases that ran past the next release since boot.
- `stack` is the painted high-water mark of each task's stack.

**Acceptance:** each task's `max` stays below its period (Motion 20 ms,
Mission 10 ms, I/O 10 ms), `ovr` does not grow during a run, and stack use
stays below the stack size.

| Run | Image | Motion max | Mission max | I/O max | Overruns | Stack use |
| --- | --- | --- | --- | --- | --- | --- |
| INT-224 before narrowing | `017f5ed` trial | 889 us | 3,716 us | 2,302 us | Motion 0; Mission and I/O +50/s | 672 / 1,400 / 480 B of 2,176 B |
| INT-225 after narrowing | `f4a4a2a` trial | 1,275 us | 5,229 us | 2,400 us | Motion 0; Mission and I/O +50/s | 664 / 1,256 / 544 B of 2,176 B |

INT-224 maxima are over the whole run: idle, bench, one line lap and one bypass;
Mission's 3,716 us came during the bypass. Stack use is Motion / Mission / I/O.
Mission and I/O overruns grow at exactly 50 a second in every segment,
idle included, while their longest body stays under 4 ms of a 10 ms period. The
rate does not follow load, so the likely cause is the 10 ms kernel tick
(`CNF_TIMER_PERIOD` in `config/config.h`) making a 10 ms task release every 20 ms;
this is not confirmed. INT-225 compares like for like, so a change in that rate
would still show.

INT-225 has no bench segment; its maxima cover idle, one line lap and two
bypasses. Against INT-224, Motion's longest body rose from 889 to 1,275 us
(6.4 percent of its 20 ms period) and I/O's from 2,302 to 2,400 us. Mission's
rose from 3,716 to 5,229 us, during the second scan and the start of its bypass:
52 percent of its 10 ms period. The overrun rate is unchanged and every stack
keeps over 900 bytes free. One run each, so these are single samples, not
worst cases.

To fill this table: flash `017f5ed` (timing added, no narrowing) with
`./flash.sh trial`, capture the USB console during a bench run, one line lap
and one bypass, then repeat on the final commit. Record each run in
[testing/integration/TEST_LOG.md](../testing/integration/TEST_LOG.md).
