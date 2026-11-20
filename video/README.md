# Videos

## Contents

- [Status](#status)
- [What to record](#what-to-record)
- [How to share](#how-to-share)
- [Video index](#video-index)
- [Pending uploads](#pending-uploads)

Short videos of tests and demos on the real hardware, showing what a log cannot, such as the robot reaching the line.

## Status

As of 2026-11-20, 17 silent videos are on YouTube as unlisted uploads. BAY has seven and Nicholas Bay has ten.

Nine more videos are ready without audio. Both channels reached YouTube’s daily upload limit.

## What to record

- **Short:** 15-60 seconds; the [full course run](../testing/system/README.md#sys-u01-full-course-run) can be as long as the run.
- **One test per video.** Show the test ID (e.g. `S2-U03`, `SYS-U01`) at the start. A separate terminal clip can show the console output for the same test.
- **Show the commit:** run `git rev-parse --short HEAD`, put it in the index, and record the flashed-image checksum in the linked test result.
- **Show the result:** keep the hardware and, if relevant, the USB TEST console in view. During battery-only trials follow the [power policy](../README.md#power-and-flashing) and use camera, scope or wireless evidence.
- **Landscape**, steady, good light. No music needed.

## How to share

1. Upload to YouTube as **Unlisted**: anyone with the link can watch, but it is not searchable. Not *Private* (the team and instructor could not open it) and not *Public*.
2. Copy the link into the index below. If the video is evidence for a test, also put the link in that test's result row in [testing](../testing/README.md).

Never commit video files: they stay in git history for good and slow every clone. `.gitignore` excludes them.

## Video index

Newest at the bottom. Never delete a recorded row.

| Test date | Subsystem | Test ID / demo | Commit | Subsystem owner ID | Clip | Link |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-11-18 | [Comms](../testing/comms/TEST_LOG.md#results) | S1-U01 | `12b3983` | 2500629 | Terminal | [Watch](https://youtu.be/ZYH8X5pl338) |
| 2026-11-18 | [Comms](../testing/comms/TEST_LOG.md#results) | S1-U02 | `12b3983` | 2500629 | Terminal | [Watch](https://youtu.be/yrx6Bqv97ds) |
| 2026-11-18 | [Comms](../testing/comms/TEST_LOG.md#results) | S1-U03 | `12b3983` | 2500629 | Terminal | [Watch](https://youtu.be/Afe4-LWxF1s) |
| 2026-11-18 | [Comms](../testing/comms/TEST_LOG.md#results) | S1-U04 | `12b3983` | 2500629 | Terminal | [Watch](https://youtu.be/zoLosirWVpc) |
| 2026-11-18 | [Comms](../testing/comms/TEST_LOG.md#results) | S1-U05 | `12b3983` | 2500629 | Terminal | [Watch](https://youtu.be/m3s9QtUCYl0) |
| 2026-11-18 | [Motion](../testing/motion/TEST_LOG.md#results) | S2-U02 | `2526ba8` | 2503973 | Test | [Watch](https://youtu.be/iEoZOhbyI6s) |
| 2026-11-18 | [Motion](../testing/motion/TEST_LOG.md#results) | S2-U03 | `184d655` | 2503973 | Test | [Watch](https://youtube.com/shorts/KEXYGgADgUk) |
| 2026-11-18 | [Motion](../testing/motion/TEST_LOG.md#results) | S2-U04 | `6f59d95` | 2503973 | Test | [Watch](https://youtu.be/Sn4kJvwNpuI) |
| 2026-11-18 | [Motion](../testing/motion/TEST_LOG.md#results) | S2-U04 | `6f59d95` | 2503973 | Terminal | [Watch](https://youtu.be/OcXPIzXcfIg) |
| 2026-11-18 | [Motion](../testing/motion/TEST_LOG.md#results) | S2-U05 | `bb0f4ce` | 2503973 | Test | [Watch](https://youtu.be/0NZW-VySh94) |
| 2026-11-18 | [Line and barcode](../testing/line_barcode/TEST_LOG.md#results) | S3-U02 | `ce16c4f+` | 2501552 | Terminal | [Watch](https://youtu.be/16t21lRjw1w) |
| 2026-11-18 | [Line and barcode](../testing/line_barcode/TEST_LOG.md#results) | S3-U04 | `5c0d4e0` | 2501552 | Test | [Watch](https://youtube.com/shorts/-9bpd8Z4s1g) |
| 2026-11-18 | [Line and barcode](../testing/line_barcode/TEST_LOG.md#results) | S3-U04 | `5c0d4e0` | 2501552 | Terminal | [Watch](https://youtu.be/C3tNZB74D6c) |
| 2026-11-18 | [Line and barcode](../testing/line_barcode/TEST_LOG.md#results) | S3-U05 | `5c0d4e0` | 2501552 | Test | [Watch](https://youtube.com/shorts/c717WRrZeYQ) |
| 2026-11-18 | [Line and barcode](../testing/line_barcode/TEST_LOG.md#results) | S3-U05 | `5c0d4e0` | 2501552 | Terminal | [Watch](https://youtu.be/Vnk85cuwu1A) |
| 2026-11-18 | [Obstacle](../testing/obstacle/TEST_LOG.md#results) | S5-U01 | `e922198` | 2502825 | Test | [Watch](https://youtu.be/_Hi5o9f7MZk) |
| 2026-11-18 | [Obstacle](../testing/obstacle/TEST_LOG.md#results) | S5-U01 | `e922198` | 2502825 | Terminal | [Watch](https://youtu.be/1lZWcA1DVao) |

## Pending uploads

The older S3-U01 and S3-U03 drafts on BAY contain audio. Their silent replacements are among the pending uploads.

- `SYS-U01.mov`
- `S3-U01.MOV`
- `S3-U02.MOV`
- `S3-U03.MOV`
- `S3-U03-terminal.mov`
- `S5-U02.mp4`
- `S5-U02-terminal.mov`
- `S5-U03.MOV`
- `S5-U03-terminal.mov`
