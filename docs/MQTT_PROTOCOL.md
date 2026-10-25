# MQTT binary protocol, schema version 1

Implemented by `app_program/subsystem1/mqtt_frame.[ch]` and tested by `testing/pure/test_mqtt_frame.c`. Field widths follow the [data representation](DATA_REPRESENTATION.md) rows; rows marked pending there carry provisional widths here and change only through the data-contract review (#28).

Rules for every frame:

- Little-endian multi-byte fields. Fields are written with explicit masks and shifts, never by copying a struct.
- Byte 0: bits 7-4 version (1), bits 3-0 type.
- Length is exact for the type. A receiver rejects a wrong length, version or type, any set reserved bit, and any out-of-range field. Nothing is partially accepted.
- Networking never drives, starts or stops the robot. The only command that changes robot state is the mapping, and only while stopped.

## Types

| Type | Value | Direction | Length (bytes) |
| --- | --- | --- | --- |
| Telemetry | 1 | robot to PC, every 500 ms | 18 |
| Event | 2 | robot to PC | 11 |
| Snapshot reply | 3 | robot to PC, answers a request; body as telemetry | 18 |
| Mapping | 4 | PC to robot | 5 |
| Acknowledgement | 5 | robot to PC | 3 |
| Snapshot request | 6 | PC to robot | 2 |

## Telemetry and snapshot reply (18 bytes)

| Byte | Bits | Field | Range |
| --- | --- | --- | --- |
| 0 | 7-0 | version and type | `0x11` telemetry, `0x13` reply |
| 1 | 7-0 | sequence | 0-255, wraps |
| 2 | 3-0 | mission state | 0-15 |
| 2 | 5-4 | motion status | 0 IDLE, 1 RUNNING, 2 DONE, 3 FAILED |
| 2 | 7-6 | reserved | 0 |
| 3 | 5-0 | motion action ID | 0-63 |
| 3 | 7-6 | reserved | 0 |
| 4-8 | 7-0 each | fault bytes: Motion, Line/Barcode, Terrain, Obstacle, Network | owner bit positions |
| 9-10 | 15-0 | left encoder count | wrapped, `ENC_COUNT_BITS` wide |
| 11-12 | 15-0 | right encoder count | wrapped |
| 13 | 7-0 | hump count | 0-255 saturating |
| 14 | 6-0 | maximum hump height code | 7-bit result, scale D06 |
| 14 | 7 | hump height valid | |
| 15-16 | 12-0 | front range, mm | 0-8190; 8191 invalid |
| 15-16 | 13 | front range valid | no echo is not clear space |
| 15-16 | 15-14 | reserved | 0 |
| 17 | 4-0 | last decoded barcode letter | 0-25 (A-Z), 31 none |
| 17 | 7-5 | action it maps to | 0 unmapped, 1 left, 2 right, 3 straight, 4 U-turn |

## Event (11 bytes)

| Byte | Field |
| --- | --- |
| 0 | version and type, `0x12` |
| 1 | bits 3-0 kind: 1 barcode, 2 hump, 3 obstacle, 4 turn or segment; bits 7-4 reserved 0 |
| 2 | arg: letter (barcode), height code (hump), action (turn) |
| 3-4 | time, low 16 bits of kernel milliseconds |
| 5-6 | left encoder count at the event |
| 7-8 | right encoder count at the event |
| 9-10 | arg2: action (barcode), width mm (obstacle) |

## Mapping (5 bytes) and acknowledgement (3 bytes)

Mapping: byte 0 `0x14`, byte 1 sequence, bytes 2-4 a 24-bit little-endian value whose bits 4-0, 9-5, 14-10 and 19-15 are the letters for left, right, straight and U-turn (A=0 .. Z=25); bits 23-20 are reserved 0. The four letters are distinct.

Acknowledgement: byte 0 `0x15`, byte 1 the sequence being answered, byte 2 the result: 0 installed, 1 busy (robot not stopped), 2 invalid letters, 3 malformed frame. Receipt alone is never acknowledged: the acknowledgement follows installation by Mission.

Snapshot request: byte 0 `0x16`, byte 1 sequence.

## Golden vectors

Used by the host tests and by the PC client (#42).

| Frame | Bytes |
| --- | --- |
| Telemetry | `11 2A 32 05 01 02 04 08 03 34 12 78 56 03 F8 D0 30 61` |
| Event (barcode Z, action 2) | `12 01 19 E8 03 10 00 11 00 02 00` |
| Mapping A left, B right, C straight, D U-turn, sequence 7 | `14 07 20 88 01` |
| Acknowledgement installed, sequence 7 | `15 07 00` |
| Snapshot request, sequence 9 | `16 09` |

## Sizes against the baseline

| Message | Baseline on `main` (JSON) | Binary |
| --- | --- | --- |
| Telemetry | up to 768 bytes (payload bound `S1_TELEMETRY_PAYLOAD_MAX`) | 18 bytes |
| Mapping | up to 512 bytes on `uat` | 5 bytes |

Baseline figures come from the [CR baseline](evidence/CR_BASELINE.md) and the #22 census; the binary sizes are the exact frame lengths above. Message-size comparison under identical builds is completed in #55.
