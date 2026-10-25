# Data representation

This file is the Week 13 data table required by [CHANGE-REQUIREMENT](CHANGE-REQUIREMENT.md) and the [minimum-bit rules](CODING_STANDARD.md#minimum-bit-data). Each owner fills the rows for their subsystem and the team reviews the layout jointly before any field is converted (issue #28). A row marked `pending` means the owner has not yet supplied the value; nothing is assumed. A row marked **proposed** was drafted from the implementation on the issue branch named in the row; it is not signed off until its owner confirms it in the issue thread.

Rules each row must satisfy:

- Bits = `ceil(log2(number of valid values + invalid codes))`.
- Combine fields into one storage unit only when they share a writer.
- Wire format uses explicit masks and shifts; in-RAM packed state may use bit-fields behind accessors.
- Stored floating-point and timestamp values need a bit-budget justification or fixed point.
- Loop counters and short-lived temporaries are exempt.

Only rows already decided by [ARCHITECTURE](ARCHITECTURE.md) or [PROJECT_SPECS](PROJECT_SPECS.md) carry values. Source for each: S5 scan state and S4 height in [TRIBUNAL1](TRIBUNAL1.md); mapping in the [post-Tribunal decisions](ARCHITECTURE.md#post-tribunal-decisions).

## S1 Communications

Frame layouts are defined in [MQTT_PROTOCOL](MQTT_PROTOCOL.md) (issue #39). Widths below are **proposed** and need Shaun's sign-off.

| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Frame header: version | byte 0 bits 7-4 | S1 | 1 | - | no | - | - | other values rejected | frame rejected | 4 | mask/shift |
| Frame header: type | byte 0 bits 3-0 | S1 | 1-6 | - | no | - | - | other values rejected | frame rejected | 4 | mask/shift |
| Sequence | byte 1 | S1 | 0-255 | count | no | 1 | none | none | wraps modulo 256 | 8 | byte |
| Telemetry: mission state / motion status / action ID | bytes 2-3 | S1 | 0-15 / 0-3 / 0-63 | - | no | - | - | out of range rejected on encode and decode | rejected | 4 / 2 / 6 | mask/shift |
| Telemetry: fault bytes | bytes 4-8 | S1 | one byte per owner | - | no | - | - | - | - | 5 x 8 | bytes |
| Telemetry: encoder counts | bytes 9-12 | S1 | wrapped counts | counts | no | 1 | none | none | wraps (modular) | 2 x 16 | little-endian |
| Telemetry: hump count / maximum height | bytes 13-14 | S1 | 0-255 / 0-126 | count / D06 scale | no | D06 | see S4 | 127 invalid; bit 7 valid flag | count saturates; height saturates at 126 | 8 / 7 + 1 | mask/shift |
| Telemetry: front range | bytes 15-16 | S1 | 0-8190 | mm | no | 1 | nearest mm | 8191 invalid; bit 13 valid flag | saturates to invalid | 13 + 1 | mask/shift |
| Telemetry: last barcode letter / action | byte 17 | S1 | 0-25 or 31 none / 0-7 | - | no | - | - | 31 no letter | - | 5 / 3 | mask/shift |
| Event record: kind / arg / time / position / arg2 | 11-byte frame | S1 | kind 1-4; time low 16 bits of ms | ms, counts | no | 1 | none | kind 0 and 5-15 rejected | time wraps modulo 65536 | 4 / 8 / 16 / 2 x 16 / 16 | little-endian |
| Mapping acknowledgement `{sequence, result}` | 3-byte frame | S1 | sequence 0-255; result 0-3 | - | no | - | - | result above 3 rejected | sequence wraps | 8 / 8 (2 used) | bytes |

## S2 Motion

Encoder width is **provisional** (16 bits, `ENC_COUNT_BITS`) until D03 supplies counts per revolution and the longest segment; the change is one constant in `robot_config.h`. Branches: #29, #31.

| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Encoder count (per wheel) | wrapping counter | Encoder ISR (core 1) | wrapped | counts | no | 1 (mm per count from D03 geometry) | none | none | wraps; consumers use modular subtraction | 16 (provisional, D03) | unsigned fixed-width |
| Motion command: mode, speed, steering or target, action ID, sample time | mutex-copied struct (> 32 bits) | Mission | mode 0-3; speed 0-300 mm/s; steering -1..+1; target in mm or degrees; action ID 0-63 | mm/s, degrees, ms | mixed | pending | pending | invalid fields brake | stale sample brakes | pending (#51) | struct today; packed form pending |
| Motion feedback: status (IDLE/RUNNING/DONE/FAILED), action ID | feedback word | Motion | status 4 values; action ID 0-63 | - | no | - | - | - | ID wraps; ID 1 reserved for FOLLOW | 2 + 6 (action width provisional) | one word, single store |

## S3 Line, barcode and Mission

Branches: #35, #36, #37. Rows marked pending are sensor fields converted by #52 after Nicholas confirms them.

| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Barcode mapping letter (L, R, S, U) | mapping word | Mission (on valid mapping) | A-Z, 26 values, all four distinct | letter index | no | 1 | none | none (boot state is UNMAPPED) | rejected before install | 5 each, 20 for four | 4 x 5-bit fields |
| Mapping state | mapping word bit 20 | Mission | UNMAPPED, MAPPED | - | no | - | - | - | - | 1 | flag |
| Line sensor readings (2 analog) | S3 snapshot struct | S3 | 0-4095 ADC counts, normalised -1..+1 | ADC counts | no (counts) | 1 | none | validity flags | averaged ADC saturates at 4095 | 12 each (counts); normalised pending | struct today; pending (#52) |
| Barcode edge ring entry | ISR ring entry | Barcode ISR | time 32 bits; counts 2 x 16; level 1 | us, counts | no | 1 | none | overflow sets a fault bit | new edge dropped, fault set | 32 + 32 + 1 | struct |
| Decoded barcode event | event queue entry (8 deep) | S3 | letter 0-25; action 0-4; orientation 0-1; time; position | - | no | - | - | unmapped action 0 | queue full sets a fault bit, newest dropped | 5 + 3 + 1 + 32 + 32 | struct; packed form pending (#52) |
| Mission state | state value | Mission | 7 states (CALIBRATE, READY, FOLLOW, JUNCTION_TURN, BYPASS, COMPLETE, STOP) | - | no | - | - | - | - | 3 (4 on the wire) | one encoded value |
| Safety latched cause | cause word | Safety | 4 cause bits | - | no | - | - | 0 = not latched | - | 4 | bit mask |

## S4 Terrain

Scale is **provisional** (1 mm per count, `HUMP_MM_PER_COUNT`) until D06 measures reference humps. Branches: #45, #46.

| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Hump height, working value | task-local | Terrain | 0-6553.5 | mm | no | 0.1 mm | nearest | - | saturates at 65535 | 16 | fixed point |
| Hump height, reported result | terrain word bits 0-6 | Terrain | 0-126 | mm (D06 scale) | no | 1 mm per count (provisional) | nearest | 127 | saturates at 126, never reaches the invalid code | 7 | unsigned fixed point |
| Hump count | terrain word bits 14-21 | Terrain | 0-255 | count | no | 1 | none | - | saturates at 255 | 8 | unsigned |
| Maximum hump height | terrain word bits 7-13 | Terrain | 0-126 | same as reported result | no | same as reported result | nearest | 127 | saturates at 126 | 7 | unsigned fixed point |
| Terrain state / height valid / calibrated | terrain word bits 22-26 | Terrain | 5 states / flag / flag | - | no | - | - | - | - | 3 / 1 / 1 | mask/shift |

## S5 Obstacle

Branches: #48, #49. Widths are **proposed**.

| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Scan state | scan word bits 0-2 | Obstacle | 8 states: IDLE, SETTLING, COARSE, FINE, DONE, FAILED, CANCELLED, SIDE_WATCH | - | no | - | - | out-of-range code decodes as FAILED | - | 3 | one encoded value |
| Scan ID | scan word bits 3-6 | Mission request | 0-15 | - | no | - | - | - | wraps | 4 | mask/shift |
| Range, front / side | echo word bits 0-12 + valid flag + time | Obstacle | 0-8190 | mm | no | 1 | nearest mm | 8191 and valid flag clear: no echo, never clear space | saturates to invalid | 13 + 1 + 16 (time low 16 bits) | mask/shift |
| Quantized obstacle profile | profile snapshot (mutex copy) | Obstacle | distances 0-65535 mm; closest angle -60..+60 | mm, degrees | mixed | 1 | nearest | valid flag clear | saturates at 16 bits | 5 x 16 + 1 | struct; packed form pending (#54) |
| Obstacle width / depth | profile width; depth from encoder travel | Obstacle / Mission | 0-65535 | mm / counts | no | 1 | nearest | - | saturates | 16 | unsigned |
| Servo angle | command | Obstacle | -60..+60 | degrees | yes | 1 | nearest | - | clamped to the sweep limit | 7 | signed |

## Mission and Safety

Safety publishes the aggregate for telemetry; it does not write the owner bytes.

| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Combined fault mask | read on demand (`rd_fault_aggregate`) | Safety (read only) | union of the four stop-capable owner bytes | - | no | - | - | - | - | 32 (Network byte reported separately) | bitmask |
| Stop latch cause | cause word | Safety | 4 cause bits (GP21, fault, stale line, stale motion) | - | no | - | - | 0 = not latched | - | 4 | bit mask |

## Fault bitmask

One byte per owner. Each owner sets and clears only its own bits; Safety reads every byte each release. The bit positions below are **proposed** by `app_program/common/robot_data.h` (issue #27) and need each owner's confirmation. Network bits are report-only and never stop motion.

| Byte | Writer | Bit 0 | Bit 1 | Bit 2 | Bit 3 |
| --- | --- | --- | --- | --- | --- |
| Motion | S2 | motor stall | encoder fault | - | - |
| Line/Barcode and Mission | S3 | IR invalid | line lost | barcode capture overflow | barcode event overflow |
| Terrain | S4 | IMU fault | calibration failed | impact | - |
| Obstacle | S5 | no echo | servo fault | - | - |
| Network | kernel task, core 0 | Wi-Fi lost | broker lost | - | - |

## Sign-off record

Rows marked **proposed** or **pending** stay open until their owner confirms them in the issue #28 thread. This file does not record an approval that has not been given.
