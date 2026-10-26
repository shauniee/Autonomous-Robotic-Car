# Testing and evidence

Keep subsystem cases/results in `subsystemN/TEST_LOG.md`, cross-subsystem cases in `integration/TEST_LOG.md`, and physical/full-product cases in `system/TEST_LOG.md`. Saved evidence goes in the corresponding `logs/` directory.

| Level | Stable IDs | Ownership / contract |
| --- | --- | --- |
| Subsystem | `S1-U01`, `S2-U01`, … | Subsystem owner |
| Integration | `INT-01`, … | Both interface owners; [system contracts](../docs/architecture.md#shared-data-rules) and [IR handoff](../docs/architecture/subsystem3.md) |
| System | `SYS-01`, …; planned `T01`–`T16` | Team; [requirements](../docs/project-specs.md) |

IDs never change or get reused, including retired template cases. Retired definitions remain in the relevant log; placeholder template rows are not observations.

## Contents

- [Run and record](#run-and-record)
- [Preserved software evidence](#preserved-software-evidence)
- [Planned integrated trials](#planned-integrated-trials)

## Run and record

1. Define the case, steps, expected result and acceptance measurements before the run. Record wiring, power, calibration and build profile.
2. For an appropriate firmware self-test, use `TEST_CHECK` with the case ID in the owning `subsystemN_test.c`; see `app_program/test_report.h`. Manual cases need hardware/operator steps and measurement evidence.
3. Use [build/flash profiles](../README.md#getting-started). TEST firmware prints `[TEST] <ID> PASS|FAIL` at startup and always inhibits motor outputs. A compiled image is not an executed pass.
4. Save actual console output/photos/scope records as `YYYY-MM-DD_<case>.txt` or appropriate image format. Start console capture before resetting the USB-only TEST board. Example from repository root, using the actual device name:

   ```sh
   cat /dev/cu.usbmodem<device> | tee testing/subsystem3/logs/YYYY-MM-DD_self-tests.txt
   # Linux: cat /dev/ttyACM0 | tee testing/subsystem3/logs/YYYY-MM-DD_self-tests.txt
   ```

   Windows PuTTY supports Session → Logging → All session output. Observe the [power policy](../README.md#getting-started) during physical trials.
5. Append a result row: date, ID, exact committed revision, tester, PASS/FAIL, measurements and evidence. Record the flashed-image checksum. Commit records and evidence together. Existing precommit checks instead identify the tested working tree through their source manifests plus base HEAD; base HEAD alone is insufficient.
6. Record a failure and its reason; after a fix, append a new result. Never edit/delete a historical result or overwrite its evidence filename. Rerun affected subsystem/integration cases before merging, and the applicable full set before a demonstration.

## Preserved software evidence

Host-only tests for kernel-free modules run with `testing/pure/run.sh` (no fake kernel). The older 2026-10-08 fake-kernel/hardware/network and laptop-loopback results remain historical software evidence; their recorded methods are not current run instructions. They do not establish GPIO performance, physical motion or an actual Pico connection.

Firmware self-tests remain: S1-U04 (bounded encoding), S1-U06/U07 (status-only requests/network status), S2-U03 (Motion policy), S3-U02 (normalization/progress) and S3-U06 (P guidance). S3-U03 remains the retired line-PID case; later P/capture/recovery cases retain their IDs. Network snapshots never count as sensor/watchdog progress.

Actual wireless checks are SYS-09/10 through [wireless setup](../README.md#getting-started). Save Pico-originated subscriber output separately from synthetic/laptop transport evidence. A slow telemetry stream cannot prove short stop/recovery deadlines; use camera/scope measurements.

## Planned integrated trials

All remain pending until actual measurements/results are recorded. This retains the integrated design's IDs without treating proposed checks as passes.

| ID | Scenario / evidence |
| --- | --- |
| T01 | Electrical release: markings, continuity, loaded rails/current, signal highs, shared contacts and jumper |
| T02 | Boot/stop/reset in every motion mode; output-brake timing and physical stopping separately |
| T03 | Straight/curve/junction tracking under lighting, surface and speed variation |
| T04 | Known barcode scales/speeds, partial/unknown/reverse/duplicate frames and capture/event overflow |
| T05 | Four encoder actions: repeated distance/angle accuracy, completion, timeout and obstacle cancellation |
| T06 | Labelled stationary/acceleration/turn/climb/descent/impact traces; no raw gyro claim |
| T07 | Measured reference humps, slip/dynamic/incomplete rejection and invalid estimate after valid maximum |
| T08 | Known forward ranges/materials/angles; no/late/out-of-range echo and repeat timing |
| T09 | Moving scan rejection, coarse/fine profile, separate front age and restored-center acquisition |
| T10 | Vehicle envelope/course boundary, both bypass sides, blocked paths and original-line versus wrong-crossing recovery |
| T11 | Controlled line loss, interrupted search, obstacle during search and bounded timeout |
| T12 | Actual mission telemetry/status subscription, units/validity and rejection of prohibited/malformed commands |
| T13 | WiFi/broker loss/reconnect under sensor/scan load; healthy local control and no stale replay |
| T14 | Stale/frozen producers, encoder/stall faults, expired command and full diagnostic/navigation/capture buffers |
| T15 | Concurrent load: releases/WCET, missed deadlines, stop latency, queue occupancy and stack/memory usage |
| T16 | Repeated complete-course rehearsal with navigation, humps, obstacles, telemetry and recovery |

Acceptance targets/unknown values are owned by [project-specs](../docs/project-specs.md#pending-decisions), not inferred from a case name. Demonstration links belong in the [video index](../docs/week13-plan.md#video-index) and the corresponding result row.
