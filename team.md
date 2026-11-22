# Team, hardware and workflow

## Contents

- [1. Members and subsystems](#1-members-and-subsystems)
- [2. Pins](#2-pins)
  - [Supply and wires](#supply-and-wires)
  - [Wiring conflicts](#wiring-conflicts)
  - [Physical header map (W/WH; header numbers differ from GPIO numbers)](#physical-header-map-wwh-header-numbers-differ-from-gpio-numbers)
- [3. Task priorities and kernel object budget](#3-task-priorities-and-kernel-object-budget)
- [4. Interfaces between subsystems](#4-interfaces-between-subsystems)
- [5. Data representation](#5-data-representation)
  - [Reading the field tables](#reading-the-field-tables)
  - [Motion and encoder](#motion-and-encoder)
  - [Line and barcode](#line-and-barcode)
  - [Obstacle](#obstacle)
  - [Terrain](#terrain)
  - [Mission and Safety](#mission-and-safety)
  - [Comms](#comms)
  - [Queues, rings and messages](#queues-rings-and-messages)
  - [MQTT binary protocol, schema version 3](#mqtt-binary-protocol-schema-version-3)
    - [Wheel trace (15 bytes)](#wheel-trace-15-bytes)
    - [Barcode trace (18 bytes)](#barcode-trace-18-bytes)
    - [Terrain trace (14 bytes)](#terrain-trace-14-bytes)
    - [Line trace (17 bytes)](#line-trace-17-bytes)
    - [Telemetry and snapshot reply (18 bytes)](#telemetry-and-snapshot-reply-18-bytes)
    - [Event (11 bytes)](#event-11-bytes)
    - [Mapping, acknowledgement, snapshot request](#mapping-acknowledgement-snapshot-request)
    - [Golden vectors](#golden-vectors)
  - [Fault bitmask](#fault-bitmask)
  - [Memory](#memory)

Use this guide to identify who owns a subsystem, trace a signal to its GPIO pin, and interpret the bits stored in RAM or sent over MQTT. Setup, flashing and task scheduling are in the [codebase guide](README.md).

A **subsystem** is one student's folder (may hold several tasks and peripherals); a **task** is a thread of execution; a **peripheral** is a hardware block. Subsystem owners retain responsibility for their folders; coordinate firmware changes with the owner. All members join integration, debugging, system testing and demonstration.

## 1. Members and subsystems

| # | Subsystem | Student ID | GitHub | Code | Tests | Guide |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | **Shaun, Communications:** WiFi/MQTT, telemetry, reporting | 2500629 | shauniee | [app_program/comms/](app_program/comms/) | [testing/comms/](testing/comms/) | [comms](app_program/comms/README.md) |
| 2 | **Mahesha, Motion:** motors, encoders, wheel PID, motion APIs | 2503973 | itsjustmdawg | [app_program/motion/](app_program/motion/) | [testing/motion/](testing/motion/) | [motion](app_program/motion/README.md) |
| 3 | **Nicholas, Line, Barcode, Mission integration:** IR, barcode, coordination, integration | 2501552 | nicholas-bay | [line_barcode](app_program/line_barcode/), [mission](app_program/mission/) | [testing/line_barcode/](testing/line_barcode/) | [line_barcode](app_program/line_barcode/README.md) |
| 4 | **Darrius, Terrain:** IMU processing, motion classification, hump measurement | 2500360 | Galaxius03 | [app_program/terrain/](app_program/terrain/) | [testing/terrain/](testing/terrain/) | [terrain](app_program/terrain/README.md) |
| 5 | **Jia Jing, Obstacle:** sonar, servo scanning, obstacle profiling | 2502825 | JJLOLLOL | [app_program/obstacle/](app_program/obstacle/) | [testing/obstacle/](testing/obstacle/) | [obstacle](app_program/obstacle/README.md) |

Stable test IDs/measurements: [testing](testing/README.md). Demonstrations: [video](video/README.md).

**Implementation status.** In the submitted firmware, Motion drives the wheels,
Mission coordinates line following, barcode turns and obstacle bypass, and I/O
runs Terrain and Reporting. All 22 subsystem tests have recorded passes. The
full-course result and the revision tested are listed in the [system test log](testing/system/TEST_LOG.md#results).
Nicholas is the contact for pending decisions and approvals.

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
| GP12 | Jia Jing | SG90 Servo 1 signal S, orange. Obstacle drives 50 Hz PWM for sonar positioning |
| GP16 | Unclaimed | Grove 4 first signal; barcode IR DOUT until 2026-11-09 |
| GP17 | Nicholas | Right line IR DOUT, digital input (Grove 4 second signal on the carrier); AOUT not connected |
| GP18 | Nicholas | Onboard WS2812 RGB LEDs 0 and 1 show the mission state; PIO0 state machine 0, Mission task only |
| GP20 | Nicholas | Active-low start. A fresh press starts a ready course run, requests a stationary scan in TEST, or runs one move in a motion bench image |
| GP21 | Nicholas | Active-low stop/cancel. A fresh press and eligible release resets the stop latch after braking |
| GP22 | Nicholas | Onboard buzzer; not used by the firmware |
| GP26 | Nicholas | Left line IR DOUT, Grove 5 / digital input, ADC not used; AOUT not connected; Grove 6 shared GP26 contact disconnected |
| GP27 | Nicholas | Barcode IR DOUT, digital input with edge interrupt, ADC not used; AOUT not connected |
| GP28 | Mahesha | Left encoder A, Grove 7, red; ADC2-capable but used digitally; open battery jumper |

User confirmation 2026-10-29: physical wiring matches this allocation and sonar electrical behavior is confirmed; motor/encoder polarity is not confirmed. Nicholas, 2026-11-05: wiring checked and unchanged, and no AOUT wire is connected on any of the three IR sensors. This does not append a measured test result. Nicholas, 2026-11-09: rewired to left line DOUT GP26, barcode DOUT GP27 and right line DOUT GP17; the connector used for each lead was not recorded. On 2026-11-18 both wheels turned forward and both encoder counts rose during the wheels-raised direction check. [Motion test S2-U01](testing/motion/TEST_LOG.md#results)

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

Lower priority numbers run first. `TP_PRC1` selects core 0 and `TP_PRC2` selects
core 1.
[Scheduling rationale](README.md#tasks) · [Configured values](app_program/common/robot_config.h)

| Application object | Allocation / ownership |
| --- | --- |
| Tasks | Three application tasks. The TEST image starts Motion and I/O first, then starts Mission after the Comms, Motion and Terrain startup tests. |
| Mutexes | Four: Motion command/health, line snapshot, Obstacle profile, cross-core events |
| Event flags | One persistent Motion stop/inhibit flag |
| Static rings | Barcode capture and events, Comms events and frame bridge, and diagnostic buffers listed in [Queues, rings and messages](#queues-rings-and-messages) |
| Task stacks | 2,048 user-stack bytes per task. Trace images supply three painted 2,176-byte stacks including the system portion |

Limits in [config/config.h](config/config.h) are total capacities, including
kernel/driver objects. The inventory above counts application allocations. Required control initialization
failure inhibits startup. Optional data failure is handled by the central policy;
normal mission readiness still requires operational WiFi and MQTT.

## 4. Interfaces between subsystems

Each provider owns private storage and exposes coherent accessors. The subsystem guides list their APIs. [Shared-data rules](README.md#shared-data-rules)

| Path | Implemented hand-off |
| --- | --- |
| Mission to Motion | One latest packed command, bounded mutex copy; Motion owns feedback and stop latch |
| Local GP21 to Motion | Independent input check, persistent stop; no command-mutex/telemetry dependency |
| line_barcode evidence to Mission | Bounded in-task sensing/decoder steps; retain original acquisition times |
| Mission to Obstacle | Mission steps the sonar and servo state machine and reads timestamped ranges and the completed scan profile |
| Terrain to Mission/Motion | I/O publishes hump results and Terrain faults. The lifted fault stays pending until the lift clears and Motion acknowledges braking |
| I/O to Mission mapping | One request in flight. Mission installs a mapping in READY or STOPPED. Junction-test requests use the same hand-off to queue a direction during eligible line following. I/O acknowledges the result |
| Mission/Terrain to Reporting | Producers append events to the shared ring under its mutex. I/O drains it for MQTT reporting |
| Reporting to kernel MQTT | Opaque bounded TX/RX bridge; only WiFi context invokes networking APIs |

Record changed interface layouts before coding and record whole-robot results in
[testing/system/TEST_LOG.md](testing/system/TEST_LOG.md). A valid
software copy or brake acknowledgement is not physical stationarity.

## 5. Data representation

Read the **Bits** column as *logical bits / stored bits*. Logical width counts the bits needed to distinguish the allowed values, including any invalid code. Storage width includes the byte or word the processor actually reads and writes. The last column identifies who can change a field and how another task gets a consistent copy.

| Example from the firmware | How to read it |
| --- | --- |
| Sonar range | Distances 0–8190 mm and the invalid code 8191 give 8,192 encodings, exactly 13 bits. Bit 13 holds the valid flag. Together they use 14 bits of a 16-bit range word. |
| Barcode widths | A width up to 400 encoder counts needs 9 bits because 8 bits stop at 255. The decoder packs 25 widths into 225 bits, or 29 bytes rounded up. The rest of the decoder state has its own fields. |
| Terrain trace travel | Q8 stores millimetres in units of 1/256 mm. For example, 3.5 mm is stored as 896. The MQTT trace sends the whole-millimetre portion in a 16-bit field. |
| States and faults | Seven mutually exclusive Mission states fit in 3 bits. Faults can coexist, so each condition has its own bit. Telemetry combines the six owners’ fault bytes into 12 defined bits. |

The field tables describe RAM unless they explicitly name a wire frame. The MQTT encoder writes each field at a fixed byte and bit offset, so firmware and laptop agree regardless of C struct padding. [MQTT byte layouts](#mqtt-binary-protocol-schema-version-3)

### Reading the field tables

- **Bits** = `ceil(log2(valid values + invalid codes))`.
- **Flags** use one-bit fields. Boolean fields normalize nonzero assignments to 1.
  Writers of unsigned one-bit fields supply 0 or 1.
- **Codes and ranges** are unsigned bit-fields (`uint8_t name : N`). A
  `_Static_assert` beside the field proves the largest enumerator or configured
  maximum fits, so a later config change fails the build instead of truncating.
  Computed values are clamped or saturated before the store.
- **Signed values** use the smallest signed type (`int8_t`, `int16_t`) outside
  bit-fields. A signed value packed into a bit-field (profile coordinates, bypass Q8 pose, Motion
  target and acceleration, turn length, recovery turn degrees, calibration
  sums) is stored unsigned in the same width, read with `rd_sext(raw, bits)`
  and written with `rd_swrap(value, bits)` (`common/robot_data.h`).
- **Shared data** has one writer per storage unit. A single value shared across
  cores or with an ISR uses the smallest lock-free C11 atomic (8, 16 or 32-bit
  load and store; Cortex-M0+ has no atomic read-modify-write). Atomics cannot be
  bit-fields, so their storage can exceed the logical bits; spare bits are zero.
  Several fields in one atomic word use `rd_field_get`/`rd_field_set` masks.
  Anything over 32 bits is copied under a mutex.
- **Times** stay `uint32_t` (ms or us) so elapsed-time arithmetic is wrap-safe;
  16-bit milliseconds would wrap every 65.5 s, inside one run. **Change
  counters** (`sequence`) stay 32-bit and are only compared for equality.
- **Floats** are per-release computation only. Terrain accumulates trace travel in unsigned Q8 fixed point.
- **Wire frames** use explicit masks and shifts, little-endian, never a struct
  copy.

### Motion and encoder

Each wheel counter wraps at 65,536. To measure travel across a wrap, each consumer subtracts its own starting count modulo 65,536.

| Field | Where | Valid range | Bits | Representation | Writer → readers, sync |
| --- | --- | --- | ---: | --- | --- |
| Encoder count, per wheel | `motion.c` `counts[2]` | 0..65535, wraps (`ENC_COUNT_BITS`) | 16 | `_Atomic enc_count_t` | encoder ISR (core 1) → all tasks; release/acquire |
| Quadrature level, per wheel | `motion.c` `previous_levels` | 0..3 | 2+2 / 8 | one byte, masks per wheel | encoder ISR only |
| Invalid edges | `motion.c` `invalid_edges` | change counter | 32 | `_Atomic uint32_t` | ISR → Motion |
| Command target | `MotionCommand_t.target` | -2000..2000 mm or -360..360 deg | 12 / 16 | `int16_t` | Mission → Motion, mutex copy |
| Command speed | `speed_action` bits 0-8 | 0..500 mm/s | 9 | mask and shift | same |
| Command action ID | `speed_action` bits 9-14 | 0..63 | 6 | mask and shift; bit 15 spare | same |
| Command mode | `mode_steering` bits 0-1 | STOP, FOLLOW, DISTANCE, TURN | 2 | mask and shift | same |
| Command steering | `mode_steering` bits 2-9 | -127..127, Q7 | 8 | mask and shift; bits 10-15 spare | same |
| Command time | `MotionCommand_t.sampled_ms` | ms, wraps | 32 | `uint32_t` | same |
| Feedback | `motion.c` `feedback` | status 0..3, action 0..63 | 8 / 32 | `rd_word_t`, bits 0-1 and 2-7 | Motion → Mission, Comms; one word |
| Health flags | `MotionHealth_t.flags` | stopped, braked, serviced once | 3 / 8 | bits 0-2 | Motion → Mission, Safety, Terrain; mutex copy |
| Health time | `MotionHealth_t.serviced_ms` | ms | 32 | `uint32_t` | same |
| Stop requests, stops serviced | `motion.c` | change counters | 32 each | `_Atomic uint32_t`; core 0 loads serviced first | core-1 callers under DI, Motion → Terrain (core 0) |
| Action target | `MotionController_t` | -2000..2000 | 12 | unsigned bit-field, sign-extended | Motion only |
| Turn style, turn arrived | `MotionController_t` | 0..3 each | 2 each | bit-fields | Motion only |
| Mode, status | `MotionController_t` | 0..3 each | 2 each | bit-fields | Motion only |
| Active, action ID | `MotionController_t` | 0..1, 0..63 | 1, 6 | bit-fields | Motion only |
| Follow base speed | `MotionController_t` | Q6 mm/s | 15 | bit-field | Motion only |
| Profile acceleration | `MotionController_t` | signed mm/s² | 11 | unsigned bit-field, sign-extended | Motion only |
| Origin and previous counts | `MotionController_t` | encoder counts | 16 each | `enc_count_t` | Motion only |
| PID integral, previous speed | `WheelPid_t` | Q4 counts, counts/s | 16 each | `int16_t` | Motion only |
| Input counts | `MotionInput_t.count[2]` | encoder counts | 16 each | `enc_count_t` | per release |
| Output status, action, stall | `MotionOutput_t` | 0..3, 0..63, flag | 2, 6, 1 | bit-fields | per release |

### Line and barcode

| Field | Where | Valid range | Bits | Representation | Writer → readers, sync |
| --- | --- | --- | ---: | --- | --- |
| IR level, per sensor | `IrRaw_t` | 0..1; 2..3 driver error | 2 each | bit-fields | per acquisition |
| IR raw word | `IrRawWord_t` | left, right, valid | 3 / 8 | `uint8_t` bits 0-2, one byte store | Mission task |
| IR fraction, per sensor | `IrReading_t` | 0 or 127 (`IR_SCALE_ONE`) | 7 each | bit-fields | per acquisition |
| IR error | `IrReading_t`, `IrTrackingOutput_t` | -127..127 | 8 | `int8_t` | per acquisition |
| Line status | `IrTracker_t`, `LineIrSnapshot_t`, `IrTrackingOutput_t` | unknown, visible, lost, broad black | 2 | bit-field | Mission task |
| Sample flags | `LineIrSnapshot_t` | black L/R, have sample, fresh, valid, guidance | 1 each | bit-fields | Mission → Mission, Safety, Comms; mutex copy |
| Sample time | `IrTracker_t`, `LineIrSnapshot_t` | ms | 32 | `uint32_t` | same |
| Edge ring entry | `barcode.c` `ring[64]` | two counts and a level | 33 | two `uint16_t` plus one bit of `ring_high` | GP27 ISR → Mission (same core); SPSC atomics |
| Edge counts, level | `barcode_edge_t` | counts, 0..1 | 16 each, 1 | `barcode_count_t`, bit-field | per pop |
| Element widths | `barcode_frame_t.widths` | 10..400 counts, 25 elements | 9 each | 29 packed bytes, masks behind `barcode_frame_width()` | Mission task |
| Chatter carry | `barcode_frame_t.carry` | 0..409 counts | 9 | bit-field | Mission task |
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
| Echo pulse width | `obstacle.c` `pulse_us` | 0..65535 us, saturates at UINT16_MAX (30,000 us acquisition timeout) | 16 | `_Atomic uint16_t` | Echo ISR → Mission |
| Consecutive misses | `obstacle_acquisition_t` | 0..3, saturates at the limit | 2 | bit-field | Mission task |
| Ray | `ObstacleRay_t` | angle ±60 deg, 0..8190 mm, valid | 8+13+1 | packed 3 bytes | Mission task |
| Profile distances | `ObstacleProfile_t` | closest, width, clearances 0..1023 mm | 10 each | bit-fields, saturate | Mission → Comms; mutex copy |
| Profile coordinates | `ObstacleProfile_t` | ±1023 mm forward and left. The unused closest-angle field stays zero | 11 each, 7 for angle | unsigned bit-fields, coordinates sign-extended | same |
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
| Filtered acceleration | `TerrainContext_t` | Q16 raw LSB | 32 each | `int32_t` | I/O only |
| Calibration sums | `TerrainCalibSum_t` | ±3276800 | 23 each | unsigned bit-fields, sign-extended, packed 9 bytes | I/O only |
| Roll baseline | `TerrainContext_t.roll_base_q8` | ±2.3 M, Q8 cdeg | 23 / 32 | `int32_t` (signed) | I/O only |
| Peak rise | `TerrainContext_t.rise_q` | 0..1200, 0.1 mm | 11 | bit-field, saturates | I/O only |
| Tilted travel | `TerrainContext_t.travel_counts` | 0..4660 counts | 13 | bit-field, saturates | I/O only |
| Pitch, roll | `TerrainContext_t` | ±9000 cdeg | 15 / 16 each | `int16_t` | I/O only |
| Hump codes, count | `TerrainContext_t` | 0..127; 0..255 | 7 each; 8 | bit-fields; `uint8_t` | I/O only |
| Flat run, calibration count | `TerrainContext_t` | 0..10; 0..100 | 4; 7 | bit-fields | I/O only |
| State, height valid, calibrated | `TerrainContext_t` | 0..2; flags | 2; 1 each | bit-fields | I/O only |
| Trace travel | `terrain.c` | mm, wraps at 65536 | 32 | `uint32_t` Q8; wire takes bits 8-23 | I/O only |

### Mission and Safety

| Field | Where | Valid range | Bits | Representation | Writer → readers, sync |
| --- | --- | --- | ---: | --- | --- |
| Published state | `mission.c` | 7 states | 3 / 8 | `_Atomic uint8_t` | Mission → Comms (core 0) |
| State, phase | `Mission_t` | 0..6; 0..4 | 3 each | bit-fields | Mission only |
| Held barcode action | `Mission_t.held` | 0..4 | 3 | bit-field | Mission only |
| Turn code, onset black, front clear | `Mission_t` | 0..3 each | 2 each | bit-fields | Mission only |
| Scan id | `Mission_t` | 0..15 | 4 | bit-field | Mission only |
| Junction and scan flags (12) | `Mission_t` | flags | 1 each | bit-fields | Mission only |
| Line integral | `Mission_t.line_i_q` | 0..45, Q7 | 6 | bit-field | Mission only |
| Action IDs | `Mission_t.action_id`, `current_action` | 1..63 | 6 each | bit-fields | Mission only |
| Turn length | `Mission_t.turn_length_mm` | ±2000 mm | 12 | unsigned bit-field, sign-extended | Mission only |
| Touch sides | `Mission_t.line_side`, `line_prev`, `cross_side` | -1, 0, +1 | 2 / 8 each | `int8_t` (signed) | Mission only |
| Steering target | `Mission_t.line_steer_q` | -127..127, Q7 | 8 | `int8_t` | Mission only |
| Reverse distances | `Mission_t.backup_mm`, `backed_mm` | 0..150 mm | 8 each | `uint8_t` | Mission only |
| Origins | `Mission_t` | encoder counts | 16 each | `enc_count_t` | Mission only |
| Ranges in | `MissionInputs_t` | front and side 0..8190 mm; clearances, profile distances 0..1023 | 13; 10 | bit-fields | per release |
| Codes in | `MissionInputs_t` | line 0..3, motion status 0..3, action 0..63, scan state 0..7, scan id 0..15, letter 0..31, edges 0..31, barcode action 0..4 | 2, 2, 6, 3, 4, 5, 5, 3 | bit-fields | per release |
| Flags in (17) | `MissionInputs_t` | flags | 1 each | bit-fields | per release |
| Counts in | `MissionInputs_t` | encoder counts | 16 each | `enc_count_t` | per release |
| Event out | `MissionOutputs_t` | kind 0..4, arg 0..127, arg2 0..1023 | 3, 7, 10 | bit-fields | per release |
| Scan id, rejection out | `MissionOutputs_t` | 0..15; 0..3 | 4; 2 | bit-fields | per release |
| Side watch angle out | `MissionOutputs_t` | ±60 deg | 7 / 8 | `int8_t` | per release |
| Flags out (9) | `MissionOutputs_t` | flags | 1 each | bit-fields | per release |
| Bypass route, work | `bypass_control_t` | 0..8; 0..3 | 4; 2 | bit-fields | Mission only |
| Bypass pose | `bypass_control_t` | Q8 mm | 21 each | unsigned bit-fields, sign-extended | Mission only |
| Bypass edge budget | `bypass_control_t.edge_counts` | 0..93192 counts | 17 | bit-field | Mission only |
| Bypass half width | `bypass_control_t.half_width_mm` | 0..511 mm | 9 | bit-field | Mission only |
| Bypass samples, flags, action ID | `bypass_control_t` | 0..7, 0..3, flags, 0..63 | 3, 2, 1, 6 | bit-fields | Mission only |
| Bypass return saw white | `bypass_control_t.return_white` | flag | 1 | bit-field | Mission only |
| Bypass geometry | `bypass_control_t` | centre, front ±1023 mm; offset, flank, heading, target unbounded | 11 / 16; 16 | `int16_t`, `uint16_t` | Mission only |
| Bypass and recovery inputs | `bypass_input_t`, `recovery_input_t` | counts; ranges 0..8190; status 0..3; action 0..63; flags | 16; 13; 2; 6; 1 | `enc_count_t`, bit-fields | per release |
| Recovery phase | `recovery_control_t`, `recovery_output_t` | 0..3 | 2 | bit-field | Mission only |
| Recovery turn | `recovery_control_t` | ±360 deg | 10 | unsigned bit-field, sign-extended | Mission only |
| Latched causes | `safety.c` `latched` | 4 cause bits | 4 / 32 | `rd_word_t` | Mission (Safety step) → Mission |
| Reset state | `safety.c` | boot, await press, await release | 2 / 8 | `uint8_t`, passed by pointer | Mission only |
| Button flags | `Button_t`, `ButtonInputs_t` | flags | 1 each | bit-fields | Mission task |
| Safety inputs | `SafetyInputs_t` | flags; ages in ms | 1 each; 32 | bit-fields; `uint32_t` | per release |
| Mapping request | `mission_mapping.c` `request_word` | letters 20, mapped 1, sequence 8, direction 3 | 32 | `rd_word_t` bits 0-31 | I/O (core 0) → Mission |
| Mapping reply | `mission_mapping.c` `reply_word` | sequence 8, result 2 | 10 / 32 | `rd_word_t` | Mission → I/O |
| Status LEDs ready | `status_led.c` `led` | flag | 1 | bit-field | Mission only |

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
| Barcode diagnostic queue | 32 | 8-byte `barcode_diagnostic_t` | Mission task only when `BARCODE_DIAGNOSTICS=1` |
| Terrain trace | 1 pending | encoded 14-byte frame | Terrain produces and Comms consumes in the same I/O task, including on the course image |
| Comms event ring | 16 | 10 bytes; 162 bytes in all | cross-core mutex, bounded wait |
| Bridge out / in | 4 / 2 | 19-byte slot | SPSC atomics, core 0 |
| Snapshots | 1 each | `MotionCommand_t` 12, `MotionHealth_t` 8, `LineIrSnapshot_t` 8, `obstacle_profile_snapshot_t` 20 (includes the 9-byte profile), range samples 2 x 16 bytes | mutex copy |

### MQTT binary protocol, schema version 3

Byte 0 identifies the frame. Its upper four bits hold the version (3), and its lower four bits hold the type. The remaining fields use explicit masks and shifts and little-endian byte order. Each type has a fixed length. Reserved bits must be zero. The retained status topic carries the text `"1"` for online and `"0"` for offline. [Firmware codec](app_program/comms/mqtt_frame.c) · [Laptop decoder](scripts/mqtt_client.py)

| Frame | Type | Direction | Bytes | Used / stored bits |
| --- | ---: | --- | ---: | ---: |
| Telemetry | 1 | Robot to laptop, every 500 ms | 18 | 135 / 144 |
| Event | 2 | Robot to laptop | 11 | 83 / 88 |
| Snapshot reply | 3 | Robot to laptop, same layout as telemetry | 18 | 135 / 144 |
| Mapping | 4 | Laptop to robot | 5 | 36 / 40 |
| Acknowledgement | 5 | Robot to laptop | 3 | 18 / 24 |
| Snapshot request | 6 | Laptop to robot | 2 | 16 / 16 |
| Junction test | 7 | Laptop to robot | 3 | 19 / 24 |
| Wheel trace | 8 | Robot to laptop | 15 | 120 / 120 |
| Line trace | 9 | Robot to laptop | 17 | 135 / 136 |
| Barcode trace | 10 | Robot to laptop | 18 | 144 / 144 |
| Terrain trace | 11 | Robot to laptop | 14 | 109 / 112 |

In schema v3 the encoder packs six RAM fault bytes into twelve bits of a two-byte wire field. Four bits are reserved. This reduced the telemetry frame from 22 to 18 bytes. Normal telemetry carries wheel counts, from which the laptop derives mean speed and direction. Wheel traces carry per-release controller measurements.

#### Wheel trace (15 bytes)

Type 8 reports the wheel controller’s targets, measurements and applied duty.
Byte 0 is `38` hex; bytes 1..4 are robot release time uint32 ms. Bytes 5..8
contain left target in bits 0..14 and right target in bits 15..29 (signed
15-bit counts/s), TEST inhibition in bit 30 and motor-output service success
in bit 31. Bytes 9..10 and 11..12 are signed 16-bit measured left/right counts/s.
Bytes 13 and 14 are signed left/right duty percentages. Invalid encodings and
rounding are owned by the [data table](#motion-and-encoder).
Output failure marks both duty fields invalid. TEST reports zero duty with
inhibition set. Targets represent computed wheel demands; serviced braking may
override them. This is software output evidence, not electrical measurement.

Motion encodes into an eight-record SPSC ring after output service and outside
its IRQ exclusion. I/O drains at most two per release, after normal reporting;
offline/full TX and full trace ring drop diagnostic samples without waiting.
Robot timestamp gaps reveal missing releases; timestamps wrap modulo 2^32.
There is no diagnostic request, movement authority or live tuning input.
Use the laptop decoder shipped with the firmware.

#### Barcode trace (18 bytes)

Type 10 is an outbound barcode diagnostic. Byte 0 is `3A`
hex, byte 1 a wrapping sequence, bytes 2..17 eight little-endian runs: bit 15 black level, bits
14..0 width in encoder counts after chatter is merged, zero for an unused slot. A full ring drops frames.

#### Terrain trace (14 bytes)

Type 11 is outbound and best effort, sent on every image for the dashboard's IMU graphs: 10 per second on flat ground, 20 on a hump. Byte 0 is `3B` hex, bytes
1..2 time ms low 16, byte 3 signed pitch in quarter degrees, byte 4 bits 0..2 state, 6 encoder valid, 7 calibrated,
byte 5 peak rise mm, byte 6 acceleration in 10 mg units, byte 7 signed roll in quarter degrees, bytes 8..10 signed X, Y and Z acceleration in 16 mg units (saturate at +/-2032 mg), byte 11 current height estimate mm, bytes 12..13 travel mm (wraps at 65536).

#### Line trace (17 bytes)

Type 9 records the line-following decision for a fresh IR sample. Byte 0 is `39` hex; bytes 1..4 are the
IR acquisition time uint32 ms. Byte 5 flags: bit 0 left black, 1 right black,
2 in broad black, 3 junction candidate open, 4 junction qualified, 5 curved entry,
6 junction locked, 7 raw barcode GP27 level. Byte 6: bits 0..3 Mission state, 4..6 junction phase,
7 zero. Bytes 7..8 and 9..10 are the low 16 bits of the left/right encoder counts.
Byte 11 is the steering law output, signed Q7. Byte 12 is the law state: the side
last touched (-1 left, +1 right, 0 none) times the integral held at the last touch transition, signed Q7. Byte 13 is the FOLLOW steering sent to Motion (0 for
other modes). Bytes 14..15: bits 0..8 commanded speed mm/s, 9..10 motion mode,
11..15 running barcode edge count (wraps). Byte 16 is, while both sensors are black, the distance in mm between
the two sensors' black onsets (254 caps it), else 255.

Mission builds one record per fresh acquisition after `mission_step` while it was
moving (so the stopping decision is included) and puts it in an eight-record SPSC
ring; I/O drains at most two per release after the wheel trace, and offline/full
TX or a full ring drops records without waiting. Sample-time gaps reveal drops.
There is no request, movement authority or live tuning input. `mqtt_client.py
monitor` prints it as a short `LINE` line (`--raw` for JSON); the dashboard does not display it.

#### Telemetry and snapshot reply (18 bytes)

| Byte | Bits | Field | Range |
| --- | --- | --- | --- |
| 0 | 7-0 | version and type | `0x31` telemetry, `0x33` reply |
| 1 | 7-0 | sequence | 0-255, wraps |
| 2 | 3-0 | mission state | 0-15 |
| 2 | 5-4 | motion status | 0 IDLE, 1 RUNNING, 2 DONE, 3 FAILED |
| 2 | 7-6 | line status | 0 unknown, 1 visible, 2 lost, 3 broad black |
| 3 | 5-0 | motion action ID | 0-63 |
| 3 | 6 | mapped | four letters installed |
| 3 | 7 | reserved | 0 |
| 4-5 | 1-0 | Motion faults: stall, encoder | [fault bitmask](#fault-bitmask) |
| 4-5 | 4-2 | Line faults: IR invalid, capture overflow, event overflow | |
| 4-5 | 7-5 | Terrain faults: IMU, calibration failed, lifted | |
| 4-5 | 8 | Obstacle fault: no echo | |
| 4-5 | 9 | Mission fault: line lost | |
| 4-5 | 11-10 | Network faults: Wi-Fi lost, broker lost | |
| 4-5 | 15-12 | reserved | zero; a set bit is rejected |
| 6-7 | 15-0 | left encoder count | wrapped, `ENC_COUNT_BITS` wide |
| 8-9 | 15-0 | right encoder count | wrapped |
| 10 | 7-0 | hump count | 0-255 saturating |
| 11 | 6-0 | maximum hump height code | 7-bit result, 1 mm per count |
| 11 | 7 | maximum valid | set once any hump completed |
| 12-13 | 12-0 | front range, mm | 0-8190; 8191 invalid |
| 12-13 | 13 | front range valid | no echo is not clear space |
| 12-13 | 15-14 | IMU calibration | 0 unknown, 1 not calibrated, 2 calibrated, 3 invalid |
| 14 | 4-0 | last decoded barcode letter, cleared when its navigation ends on the line | 0-25 (A-Z), 31 none |
| 14 | 7-5 | action it maps to | 0 unmapped, 1 left, 2 right, 3 straight, 4 U-turn |
| 15-17 | 9-0 | latest obstacle width, mm | 0 none; saturates at 1023 |
| 15-17 | 19-10 | clearance on the side chosen for the latest bypass, mm | 0 none; saturates at 1023 |
| 17 | 7-4 | reserved | zero; a set bit is rejected |

IMU calibration reports Terrain's calibration flag independently of hump results
and current sensor faults.

#### Event (11 bytes)

| Byte | Field |
| --- | --- |
| 0 | version and type, `0x32` |
| 1 | bits 3-0 kind: 1 barcode, 2 hump, 3 obstacle, 4 turn; bits 7-4 reserved 0 |
| 2 | arg: letter (barcode), height code (hump), action (turn) |
| 3-4 | time, low 16 bits of kernel milliseconds |
| 5-6 | left encoder count at the event |
| 7-8 | right encoder count at the event |
| 9-10 | arg2: action (barcode), clearance mm (obstacle); arg 1 passing left, 0 right |

In RAM the event ring stores sixteen 10-byte entries. A full ring overwrites its oldest entry. If a producer cannot acquire the mutex within its deadline, it drops the incoming event. [Comms fields](#comms)

#### Mapping, acknowledgement, snapshot request

- **Mapping (5 bytes):** byte 0 `0x34`, byte 1 sequence, bytes 2-4 a 24-bit little-endian value; bits 4-0, 9-5, 14-10 and 19-15 are the letters for left, right, straight and U-turn (A=0 .. Z=25); bits 23-20 reserved 0. The four letters are distinct.
- **Acknowledgement (3 bytes):** byte 0 `0x35`, byte 1 the sequence answered, byte 2 the result: 0 OK, 1 busy, 2 invalid, 3 malformed. Mission returns the outcome of a mapping or direction request.
- **Snapshot request (2 bytes):** byte 0 `0x36`, byte 1 sequence.
- **Junction test (3 bytes).** Send `37 sequence action` in hexadecimal layout, where action is 1 left, 2 right, 3 straight or 4 U-turn. An OK acknowledgement means Mission queued the instruction.

#### Golden vectors

Used by the host tests and `scripts/mqtt_client.py --self-test`.

| Frame | Bytes |
| --- | --- |
| Telemetry (faults: Motion stall, Line capture overflow, Terrain lifted, no echo, line lost, both Network) | `31 2A 72 45 89 0F 34 12 78 56 03 F8 D0 30 61 A4 A9 02` |
| Event (barcode Z, action 2) | `32 01 19 E8 03 10 00 11 00 02 00` |
| Mapping A left, B right, C straight, D U-turn, sequence 7 | `34 07 20 88 01` |
| Acknowledgement installed, sequence 7 | `35 07 00` |
| Snapshot request, sequence 9 | `36 09` |

### Fault bitmask

One bit per condition, one `_Atomic uint8_t` per owner, written only by that
owner. Simultaneous faults stay independent. On the wire the defined bits go
into the 12-bit fault word in this order: Motion bits 0-1, Line 2-4, Terrain
5-7, Obstacle 8, Mission 9, Network 10-11.

| Owner (writer) | Bit 0 | Bit 1 | Bit 2 |
| --- | --- | --- | --- |
| Motion (Motion) | motor stall | encoder | - |
| Line (Mission task) | IR invalid | barcode capture overflow | barcode event overflow |
| Terrain (I/O) | IMU | calibration failed | lifted |
| Obstacle (Mission task) | no echo | - | - |
| Mission (Mission) | line lost | - | - |
| Network (kernel transport) | Wi-Fi lost | broker lost | - |

### Memory

The table preserves the 2026-11-08 before/after comparison against `c26bca4`.
On `a2a6156` the RP2040 size probe reproduced every value in the After column.
[Target size probe](testing/tools/sizeof_probe.sh)

| Type | Before (bytes) | After (bytes) |
| --- | ---: | ---: |
| `MissionInputs_t` | 104 | 64 |
| `barcode_frame_t` | 62 | 38 |
| `bypass_input_t` | 60 | 40 |
| `Mission_t` | 160 | 140 |
| `MissionOutputs_t` | 32 | 20 |
| `recovery_input_t` | 24 | 16 |
| `bypass_control_t` | 64 | 56 |
| `comms_event_log_t` | 168 | 162 |
| `obstacle_range_sample_t` | 20 | 16 |
| `bypass_output_t`, `SafetyInputs_t` | 20 each | 16 each |
| `MotionInput_t`, `MotionHealth_t`, `LineIrSnapshot_t` | 12 each | 8 each |
| `MotionController_t` | 48 | 44 |
| `IrRaw_t` | 4 | 1 |

| Image | Text before → after | Static RAM (bss) before → after |
| --- | --- | --- |
| Course (`TRACE=0`) | 387,912 → 390,356 B (+2,444) | 83,836 → 83,728 B (-108) |
| Trial (`TRACE=1`) | 389,712 → 393,208 B (+3,496) | 84,284 → 90,724 B (+6,440) |

In the 2026-11-08 builds, course static RAM fell by 108 bytes, and by 28 more with the schema v3 fault word (bridge slots and the receive buffer shrink from 22 to 18 bytes: course bss 83,700 B, text 390,420 B). Most of the saving is on the Mission and
Motion stacks, which hold the per-release input and output copies. Flash grows
by about 2.4 KB, the cost of reading and writing bit-fields: 0.6 percent of the
image and well inside the 2 MB part. The trial image's extra RAM is the timing
instrumentation's static task stacks (3 x 2,176 bytes), which the kernel then
no longer allocates from its own heap; the course image has no instrumentation.
