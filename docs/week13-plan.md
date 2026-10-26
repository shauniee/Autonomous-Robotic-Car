# Week 13 plan

Deferred work for the Week 13 submission. The requirement and baseline it closes are in [project-specs](project-specs.md#4-change-in-requirement-minimum-bit-data-representation). Nothing here is started; each item names its owner.

## Contents

- [1. Remaining minimum-bit conversions](#1-remaining-minimum-bit-conversions)
- [2. Evidence to produce](#2-evidence-to-produce)
- [3. Test records to revisit](#3-test-records-to-revisit)
- [4. Video recording](#4-video-recording)
  - [What to record](#what-to-record)
  - [How to share](#how-to-share)
  - [Video index](#video-index)

## 1. Remaining minimum-bit conversions

These fields still use plain types where the [data table](project-specs.md#5-data-representation-table-week-13) asks for a justified packed form. Each owner either packs the field behind accessors or records why it is exempt (loop counters and short-lived task-local temporaries are exempt).

| Owner | Field | Current form | Table row / issue |
| --- | --- | --- | --- |
| S2 Mahesha | `MotionCommand` (mode, speed, steering/target, action ID, sample time) | struct of enum, floats and `uint32_t` (`motion_control.h`) | Motion command, #51 |
| S2 Mahesha | `MotionHealth` | struct of `uint32_t` and bools (`subsystem2.h`) | not in the table yet |
| S3 Nicholas | `S3IrSnapshot` normalised readings, steering, flags | floats, seven bools, 32-bit sequence/age (`ir_tracking.h`) | Line sensor readings, #52 |
| S3 Nicholas | `barcode_edge_t` encoder counts | 2 x 32 bits; table says 2 x 16 (`drv_barcode.h`) | Barcode edge ring entry |
| S3 Nicholas | `barcode_event_t` | 32-bit counts, `char`, two enums (`barcode_decode.h`) | Decoded barcode event, #52 |
| S3 Nicholas | Mission state | enum stored in a `uint32_t` (`mission.c`); 3 bits needed | Mission state |
| S4 Darrius | `TerrainContext` | floats, enum, bools (`terrain_algo.h`); task-local, so justify exemption or pack | Terrain rows, #53 |
| S5 Jia Jing | `ObstacleProfile` / `ObstacleRay` | seven 16-bit fields plus bool (`obstacle_profile.h`) | Quantized obstacle profile, #54 |
| S5 Jia Jing | Side-watch command angle | 16-bit field for a 7-bit signed angle (`subsystem5.c`) | Servo angle |
| S1 Shaun | Event log entries | unpacked `mqtt_event_t` in RAM (`s1_report.h`) | Event record |
| S1 Shaun | Event frame | kind uses a byte (4 bits needed); arg2 is 16 bits for a 3-bit barcode action | Event record |
| All | Fault bit positions | marked PROVISIONAL in `robot_data.h` | Fault bitmask, #28 |

Convert one field at a time and rerun `testing/pure/run.sh` after each, as Tribunal risk R3 recommends.

## 2. Evidence to produce

| Evidence | How | Owner |
| --- | --- | --- |
| RAM after | Same command and toolchain as the [baseline](project-specs.md#baseline-before-issue-22), then `arm-none-eabi-size`; record text/data/bss and the `sizeof` probes for the same structs | Shaun |
| Message size after | Exact frame lengths from [architecture](architecture.md#mqtt-binary-protocol-schema-version-2) (telemetry 23 bytes in schema v2, which added line status, mapped flag, Mission fault byte and obstacle width/depth to the 18-byte v1 frame) against the JSON baseline, under identical builds (#55) | Shaun |
| No lost functionality | All host tests pass, plus the [mission-readiness checklist](../testing/mission-readiness.md) on the robot | All |
| No unacceptable timing degradation | Integrated trial T15 in [testing](../testing/README.md#planned-integrated-trials): release jitter, missed deadlines, stop latency under load | All |

## 3. Test records to revisit

Most firmware self-tests and test logs predate the binary protocol and the fault policy. Before Week 13, each owner reviews their `testing/subsystemN/TEST_LOG.md` cases against the current code: the JSON-era S1-U04..U08 cases, fault-byte expectations (now six bytes) and any case that assumed no echo stops the robot. Retire or supersede cases by appending rows; never edit recorded results.

## 4. Video recording

Short videos of tests and demos running on the real hardware. A video shows what a log cannot, such as the robot reaching the line. List every video in the [video index](#video-index).

### What to record

- **Short:** 15-60 seconds, one test or feature per video.
- **Say what it is:** show or say the test ID (e.g. `S2-U03`, `INT-01`, `MR-12`) or the demo name at the start.
- **Show the commit:** run `git rev-parse --short HEAD`, put it in the index, and record the flashed-image checksum in the linked test result.
- **Show the result:** keep the hardware and, if relevant, the USB TEST console in view. During battery-only trials follow the [power policy](../README.md#power-and-flashing) and use camera, scope or wireless evidence.
- **Landscape**, steady, good light. No music needed.

### How to share

1. Upload to YouTube as **Unlisted**: anyone with the link can watch, but it is not searchable. Not *Private* (the team and instructor could not open it) and not *Public*.
2. Copy the link (e.g. `https://youtu.be/...`) into the index below. If the video is evidence for a test, also put the link in that test's result row in [testing](../testing/README.md).

Never commit video files: they stay in git history for good and slow every clone. `.gitignore` excludes them.

### Video index

Newest at the bottom. Never delete a recorded row. The example row is a template placeholder, not recorded evidence.

| Date | Subsystem | Test ID / demo | Commit | Student ID | Link | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| YYYY-MM-DD | S1 | S1-U03 | `abc1234` | | <https://youtu.be/...> | LED blinks at 2 Hz |
