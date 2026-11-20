# Videos

## Contents

- [Status](#status)
- [What to record](#what-to-record)
- [How to share](#how-to-share)
- [Video index](#video-index)

Short videos of tests and demos on the real hardware, showing what a log cannot, such as the robot reaching the line.

## Status

Not started; nothing has been recorded.

## What to record

- **Short:** 15-60 seconds; the [full course run](../testing/system/README.md#sys-u01-full-course-run) can be as long as the run.
- **One test per video:** each test in a `testing/<folder>/TEST_LOG.md` is one video. Say its ID (e.g. `S2-U03`, `SYS-U01`) at the start.
- **Show the commit:** run `git rev-parse --short HEAD`, put it in the index, and record the flashed-image checksum in the linked test result.
- **Show the result:** keep the hardware and, if relevant, the USB TEST console in view. During battery-only trials follow the [power policy](../README.md#power-and-flashing) and use camera, scope or wireless evidence.
- **Landscape**, steady, good light. No music needed.

## How to share

1. Upload to YouTube as **Unlisted**: anyone with the link can watch, but it is not searchable. Not *Private* (the team and instructor could not open it) and not *Public*.
2. Copy the link into the index below. If the video is evidence for a test, also put the link in that test's result row in [testing](../testing/README.md).

Never commit video files: they stay in git history for good and slow every clone. `.gitignore` excludes them.

## Video index

Newest at the bottom. Never delete a recorded row.

| Date | Subsystem | Test ID / demo | Commit | Student ID | Link | Notes |
| --- | --- | --- | --- | --- | --- | --- |
