# Data representation

This file is the Week 13 data table required by [CHANGE-REQUIREMENT](CHANGE-REQUIREMENT.md) and the [minimum-bit rules](CODING_STANDARD.md#minimum-bit-data). Each owner fills the rows for their subsystem under issue #8 and the team reviews the layout jointly before any field is converted. A row left as `pending` means the owner has not yet supplied the value; nothing is assumed.

Rules each row must satisfy:

- Bits = `ceil(log2(number of valid values + invalid codes))`.
- Combine fields into one storage unit only when they share a writer.
- Wire format uses explicit masks and shifts; in-RAM packed state may use bit-fields behind accessors.
- Stored floating-point and timestamp values need a bit-budget justification or fixed point.
- Loop counters and short-lived temporaries are exempt.

Only rows already decided by [ARCHITECTURE](ARCHITECTURE.md) or [PROJECT_SPECS](PROJECT_SPECS.md) carry values. Source for each: S5 scan state and S4 height in [TRIBUNAL1](TRIBUNAL1.md); mapping in the [post-Tribunal decisions](ARCHITECTURE.md#post-tribunal-decisions).

## S1 Communications

Frame layout is defined in `docs/MQTT_PROTOCOL.md` (issue for the binary schema).

| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Frame header: version | pending | S1 | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Frame header: type | pending | S1 | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Telemetry fields | pending | S1 | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Event record (letter + action, hump height, obstacle width/depth, turn/segment distance, time, encoder position) | pending | S1 | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Mapping acknowledgement `{sequence, result}` | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending | pending |

## S2 Motion

Encoder width depends on D03 (counts per revolution) and the longest measured segment.

| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Encoder count (per wheel) | wrapping counter | Encoder ISR | pending | counts | pending | 1 | none | none | wraps; consumers use modular subtraction | pending | unsigned fixed-width |
| Motion command: mode, speed, steering or target, action ID, sample time | one packed command | Mission | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Motion feedback: status (IDLE/RUNNING/DONE/FAILED), action ID | feedback word | Motion | status 4 values | - | no | - | - | - | - | 2 for status; action ID pending | pending |

## S3 Line, barcode and Mission

The mapping letters are the only decided fields here.


| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Barcode mapping letter (L, R, S, U) | mapping word | Mission (on valid mapping) | A–Z, 26 values, all four distinct | letter index | no | 1 | none | none (boot state is UNMAPPED, tracked separately) | rejected before install | 5 each, 20 for four | 4 × 5-bit fields |
| Mapping state | pending | Mission | UNMAPPED, MAPPED | - | no | - | - | - | - | pending | pending |
| Line sensor readings (2 analog) | pending | S3 | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Barcode edge timestamps | ISR ring entry | Barcode ISR | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Decoded barcode event | event queue entry | S3 | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Mission state | pending | Mission | pending | - | no | - | - | - | - | set by Mission issue | one encoded value |

## S4 Terrain

Scale (units per count) is set by the owner under D06.

| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Hump height, working value | task-local | Terrain | pending | pending | pending | pending | pending | pending | pending | 16 | fixed point |
| Hump height, reported result | terrain word | Terrain | pending | pending | no | pending | pending | pending | pending | 7 | unsigned fixed point |
| Hump count | pending | Terrain | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Maximum hump height | pending | Terrain | pending | pending | no | same as reported result | pending | pending | pending | 7 | unsigned fixed point |

## S5 Obstacle

The scan state is the only decided field here.


| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Scan state | obstacle word | Obstacle | 8 states (list pending) | - | no | - | - | - | - | 3 | one encoded value |
| Quantized obstacle profile | profile snapshot (mutex copy) | Obstacle | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Obstacle width / depth | pending | Obstacle | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Range, front | pending | Obstacle | pending | pending | pending | pending | pending | pending | pending | pending | pending |
| Servo angle | pending | Obstacle | pending | pending | pending | pending | pending | pending | pending | pending | pending |

## Mission and Safety

Safety publishes the aggregate for telemetry; it does not write the owner bytes.

| Field | Storage unit | Writer | Range | Unit | Signed | Scale | Rounding | Invalid code | Overflow | Bits | Representation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Combined fault mask | pending | Safety | union of the owner bytes below | - | no | - | - | - | - | pending | bitmask |
| Stop latch cause | pending | Safety | pending | pending | pending | pending | pending | pending | pending | pending | pending |

## Fault bitmask

One byte per owner. Each owner sets and clears only its own bits; Safety reads every byte each release. Bit positions are assigned when the owner confirms the list; the bits named in [ARCHITECTURE](ARCHITECTURE.md#fault-bitmask) are the starting set.

| Byte | Writer | Bits named so far | Bit positions |
| --- | --- | --- | --- |
| Motion | S2 | motor stall, encoder fault | pending |
| Line/Barcode and Mission | S3 | IR invalid, line lost, barcode capture overflow, barcode event overflow | pending |
| Terrain | S4 | IMU fault, calibration failed, impact | pending |
| Obstacle | S5 | no echo, servo fault | pending |
| Network | kernel task, core 0 | Wi-Fi lost, broker lost (report-only, never stops motion) | pending |
