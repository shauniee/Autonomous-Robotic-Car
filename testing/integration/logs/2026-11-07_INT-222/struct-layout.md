# Application struct layout audit

## Contents

- [Scope and definitions](#scope-and-definitions)
- [Padding reductions](#padding-reductions)
- [All application structs](#all-application-structs)
- [Logical subsystem budgets and wire payloads](#logical-subsystem-budgets-and-wire-payloads)
- [Validation and limits](#validation-and-limits)

## Scope and definitions

Measured 2026-11-07 for RP2040, Arm GNU C11 13.3.1, Cortex-M0+, Thumb,
soft-float, `CNF_SMP=1`, at base commit
`84e5bf5d00c3d4dea70fb78091d21f3a72f8d262` plus the source hashes in
[INT-222 measurements](layout.json).
This covers all 51 application-defined struct types in `app_program/`.
The unnamed barcode count-pair struct has 32 member/stored bits and zero
padding. The unnamed mapping duplicate flag has one member bit, seven padding
bits and eight stored bits. Imported kernel/library structs are outside this audit.

- **Member bits:** declared scalar widths and bit-field widths, recursively
  excluding padding inside nested structs. A `bool` occupies eight member bits;
  reserved codes inside integer members still count. This is not a minimum
  logical range budget.
- **Stored bits:** target `sizeof(type) * 8` for one instance, including all
  nested padding and tail alignment.
- **C padding bits:** stored minus member bits, including nested padding.
  Unused bits explicitly held inside integer members are not C padding.
- Types may be persistent, temporary or returned by value. Do not sum rows as
  RAM usage: types nest, arrays have multiple instances and some are stack locals.

Field ordering and existing bit-field grouping changed; field types, widths,
ranges, scales and atomic publication rules did not. All members retain their
existing alignment requirements. No new packed attributes or pointer storage
were introduced.

## Padding reductions

| Struct | Before stored bits | After stored bits | Bits removed |
| --- | ---: | ---: | ---: |
| `MotionController` | 384 | 352 | 32 |
| `bypass_control_t` | 512 | 480 | 32 |
| `Mission` | 1280 | 1152 | 128 |
| `MissionInputs` | 832 | 768 | 64 |
| `bypass_output_t` | 160 | 128 | 32 |
| `obstacle_acquisition_t` | 960 | 928 | 32 |

`bypass_control_t` is embedded in `Mission`: its 32-bit reduction is already
included in Mission's 128-bit reduction. These are type-size reductions, not
an aggregate peak-RAM measurement.

The course `TRACE=0` linked image changed from 83,836 to 83,832 BSS bytes:
**four static RAM bytes saved**. Data remains 424 bytes. Text grew from
387,912 to 388,140 bytes (**228 bytes**). Task-local structure sizes also shrink,
but optimized frame layout and peak live stack use need separate measurements.

## All application structs

Every numeric column below is in bits, per instance.

**Common**

| Struct | Member bits | C padding bits | Stored bits |
| --- | ---: | ---: | ---: |
| `Button` | 56 | 8 | 64 |
| `ButtonInputs` | 16 | 0 | 16 |

**Comms**

| Struct | Member bits | C padding bits | Stored bits |
| --- | ---: | ---: | ---: |
| `mqtt_telemetry_t` | 226 | 14 | 240 |
| `mqtt_event_t` | 80 | 0 | 80 |
| `mqtt_mapping_t` | 40 | 0 | 40 |
| `mqtt_ack_t` | 16 | 0 | 16 |
| `mqtt_wheel_trace_t` | 240 | 16 | 256 |
| `mqtt_line_trace_t` | 152 | 8 | 160 |
| `mqtt_terrain_trace_t` | 104 | 8 | 112 |
| `comms_packed_event_t` | 80 | 0 | 80 |
| `comms_event_log_t` | 1328 | 16 | 1344 |
| `slot_t` | 192 | 0 | 192 |
| `out_ring_t` | 832 | 0 | 832 |
| `in_ring_t` | 448 | 0 | 448 |

**Motion**

| Struct | Member bits | C padding bits | Stored bits |
| --- | ---: | ---: | ---: |
| `MotionCommand` | 80 | 16 | 96 |
| `MotionHealth` | 72 | 24 | 96 |
| `MotionController` | 338 | 14 | 352 |
| `WheelPid` | 32 | 0 | 32 |
| `MotionInput` | 96 | 0 | 96 |
| `MotionOutput` | 216 | 8 | 224 |

**Line and barcode**

| Struct | Member bits | C padding bits | Stored bits |
| --- | ---: | ---: | ---: |
| `IrRaw` | 32 | 0 | 32 |
| `IrReading` | 32 | 0 | 32 |
| `IrTracker` | 43 | 21 | 64 |
| `IrTrackingOutput` | 56 | 8 | 64 |
| `LineIrSnapshot` | 80 | 16 | 96 |
| `barcode_edge_t` | 40 | 8 | 48 |
| `barcode_event_t` | 48 | 0 | 48 |
| `barcode_frame_t` | 488 | 8 | 496 |
| `barcode_diagnostic_t` | 60 | 4 | 64 |

**Terrain**

| Struct | Member bits | C padding bits | Stored bits |
| --- | ---: | ---: | ---: |
| `Gy511RawAxes` | 48 | 0 | 48 |
| `TerrainFilterQ16` | 96 | 0 | 96 |
| `TerrainCalibSum` | 69 | 3 | 72 |
| `TerrainContext` | 299 | 21 | 320 |

**Obstacle**

| Struct | Member bits | C padding bits | Stored bits |
| --- | ---: | ---: | ---: |
| `obstacle_range_sample_t` | 160 | 0 | 160 |
| `ObstacleRay` | 22 | 2 | 24 |
| `ObstacleProfile` | 70 | 2 | 72 |
| `obstacle_range_group_t` | 92 | 4 | 96 |
| `obstacle_profile_config_t` | 96 | 0 | 96 |
| `obstacle_profile_evidence_t` | 46 | 2 | 48 |
| `obstacle_profile_snapshot_t` | 148 | 12 | 160 |
| `obstacle_acquisition_t` | 893 | 35 | 928 |

**Mission and Safety**

| Struct | Member bits | C padding bits | Stored bits |
| --- | ---: | ---: | ---: |
| `Mission` | 1098 | 54 | 1152 |
| `MissionInputs` | 768 | 0 | 768 |
| `MissionOutputs` | 210 | 46 | 256 |
| `recovery_control_t` | 80 | 16 | 96 |
| `recovery_input_t` | 192 | 0 | 192 |
| `recovery_output_t` | 56 | 8 | 64 |
| `bypass_control_t` | 442 | 38 | 480 |
| `bypass_input_t` | 456 | 24 | 480 |
| `bypass_output_t` | 112 | 16 | 128 |
| `SafetyInputs` | 136 | 24 | 160 |

Remaining padding includes natural alignment and bit-field allocation-unit
boundaries. For example, `MotionCommand` keeps a naturally aligned 32-bit time
and `MotionHealth` keeps two aligned 32-bit values. Their array strides must
respect those alignments. A global packed attribute would change access costs
and alignment guarantees; this audit does not apply one.

## Logical subsystem budgets and wire payloads

The owning [README bit tables](../../../../README.md#bits-per-subsystem) list every
currently budgeted persistent item, instance counts and subsystem subtotals.
The [data representation table](../../../../docs/team.md#6-data-representation)
owns valid ranges and logical widths. Those logical tables are partial RAM
budgets; the struct inventory above adds temporary/private types without
pretending they all have approved minimum-width contracts.

Two stale frame counts were corrected against the encoder:

- Line trace: 135 used field bits, 136 wire bits (17 bytes). The one spare bit
  is bit 7 of byte 6. This changes the partial Mission used subtotal from 1000
  to 1088 bits; no payload or code encoding changed.
- Terrain trace: 109 used field bits, 112 wire bits (14 bytes). Bits 3..5 of
  the state/flags byte are spare. Its six appended bytes were missing from
  the old 61/64-bit row.

Wire totals include the eight-bit schema header, but exclude MQTT, TCP/IP and
Wi-Fi framing. Telemetry's accepted six fault-byte slots still count as 48
contract bits although only 12 independent fault conditions are defined.
That deliberate exception is owned by the data representation table.

## Validation and limits

- All six target sizeof probes pass, including exact regression assertions
  for the reduced public types; Obstacle asserts its private acquisition size.
- The obstacle probe no longer applies `sizeof` to bit-fields, which C forbids.
  The Terrain probe no longer references the deleted `TerrainTravelQuality`.
- The existing host C/Python suite, dashboard suite, changed-line BARR check
  and deliverables check pass.
- All six profiles in `testing/build_pending_profiles.sh` compile and link:
  bench normal/TEST with trace off/on, plus course trace off/on.
- Images were not flashed. Timing, peak stack, physical behavior and on-target
  packed-data contention are not established by these checks.

To repeat all public type sizes with the Arm compiler on PATH:

```sh
sh testing/sizeof_probe.sh
```

Detailed field offsets and widths were inspected in GNU DWARF 4 debug records
(`arm-none-eabi-readelf --debug-dump=info`); the saved JSON contains extracted
measurements, not console/build output. [Integration results](../../TEST_LOG.md#results)
record the run IDs and source revision.
