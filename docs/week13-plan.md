# Week 13 plan

## Contents

- [1. Remaining minimum-bit conversions](#1-remaining-minimum-bit-conversions)
- [2. Evidence to produce](#2-evidence-to-produce)
- [3. Test records to revisit](#3-test-records-to-revisit)
- [4. Video recording](#4-video-recording)
  - [What to record](#what-to-record)
  - [How to share](#how-to-share)
  - [Video index](#video-index)

## 1. Remaining minimum-bit conversions

The [central plan](../plan.md) is the implementation roadmap; this page owns
submission evidence and video indexing only. Earlier conversion lists are stale:
remote main already contains packed Motion command/health, S3 data, Terrain
context, S5 profiles and S1 event storage. Do not redo those conversions blindly.
The [data table](project-specs.md#5-data-representation-table-week-13) owns widths;
existing size results remain source-pinned history.

Audit only fields added/changed by the restructure: seven-state Mission encoding,
mode-specific progress/age inputs, Terrain freshness, mapping hand-off, cross-core
event synchronization and new scan/recovery parameters. Justify bounds, units,
invalid codes and overflow before converting. Retain schema v2 and the full
obstacle-depth argument; it is not reducible to a three-bit barcode action.
Re-run relevant boundary tests and final target-compiler size probes after changes.

## 2. Evidence to produce

| Evidence | How | Owner |
| --- | --- | --- |
| Final RAM usage | Build command and toolchain recorded with the result (the [historical baseline](project-specs.md#historical-baseline-issue-22-not-required) command is a usable template), then `arm-none-eabi-size`; record text/data/bss and `sizeof` probes for the converted structs | Shaun |
| Final message size | Exact frame lengths from [architecture](architecture.md#mqtt-binary-protocol-schema-version-2) (telemetry 23 bytes in schema v2, which added line status, mapped flag, Mission fault byte and obstacle width/depth to the 18-byte v1 frame) and inter-task message sizes, recorded with the exact commit (#55) | Shaun |
| No lost functionality | Relevant host tests and four profiles pass, plus the [mission-readiness checklist](../testing/mission-readiness.md) on the robot | All |
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
