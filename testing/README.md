# Testing and evidence

## Contents

- [Run and record](#run-and-record)
- [Qualifying lap records](#qualifying-lap-records)
- [Preserved software evidence](#preserved-software-evidence)
- [Planned integrated trials](#planned-integrated-trials)

Keep subsystem cases/results in `<name>/TEST_LOG.md`, cross-subsystem cases in `integration/TEST_LOG.md`, and physical/full-product cases in `system/TEST_LOG.md`. Saved evidence goes in the corresponding `logs/` directory.

| Level | Stable IDs | Ownership / contract |
| --- | --- | --- |
| Subsystem | `S1-U01`, `S2-U01`, … | Subsystem owner |
| Integration | `INT-01`, … | Both interface owners; [system contracts](../docs/architecture.md#shared-data-rules) and [line and barcode](../app_program/line_barcode/README.md) |
| System | `SYS-01`, …; planned `T01`–`T16` | Team; [requirements](../docs/project-specs.md) |

IDs never change or get reused, including retired template cases. Test IDs keep their old prefix: S1 is comms, S2 motion, S3 line_barcode, S4 terrain and S5 obstacle. Retired definitions remain in the relevant log; placeholder template rows are not observations.

## Run and record

1. Define the case, steps, expected result and acceptance measurements before the run. Record wiring, power, calibration and build profile.
2. For an appropriate firmware self-test, use `TEST_CHECK` with the case ID in the owning `<name>_test.c`; see `app_program/test_report.h`. Manual cases need hardware/operator steps and measurement evidence.
3. Use [build/flash profiles](../README.md#getting-started). TEST firmware prints `[TEST] <ID> PASS|FAIL` at startup and always inhibits motor outputs. A compiled image is not an executed pass.
4. Save actual console output/photos/scope records as `YYYY-MM-DD_<case>.txt` or appropriate image format. Start console capture before resetting the USB-only TEST board. Example from repository root, using the actual device name:

   ```sh
   cat /dev/cu.usbmodem<device> | tee testing/line_barcode/logs/YYYY-MM-DD_self-tests.txt
   # Linux: cat /dev/ttyACM0 | tee testing/line_barcode/logs/YYYY-MM-DD_self-tests.txt
   ```

   Windows PuTTY supports Session → Logging → All session output. Observe the [power policy](../README.md#getting-started) during physical trials.
5. Append a result row: date, ID, exact committed revision, tester, PASS/FAIL, measurements and evidence. Record the flashed-image checksum. Commit records and evidence together. Existing precommit checks instead identify the tested working tree through their source manifests plus base HEAD; base HEAD alone is insufficient.
6. Record a failure and its reason; after a fix, append a new result. Keep old rows and never overwrite a capture, photo or measurement. Do not save build or host output: the result row and commit are enough. Gzip captures over 1 MB. Write-ups and output from before 2026-11-03 are at git tag `test-logs-full-2026-11-03`. Rerun affected subsystem/integration cases before merging, and the applicable full set before a demonstration.

## Preserved software evidence

Host-only tests for kernel-free modules run with `testing/pure/run.sh` (no fake kernel). `run.sh` passes `-DHOST_TEST`, which hides the kernel and driver parts of merged files such as `line.c`, `barcode.c` and `motion.c`. The older 2026-10-08 fake-kernel/hardware/network and laptop-loopback results remain historical software evidence; their recorded methods are not current run instructions. They do not establish GPIO performance, physical motion or an actual Pico connection.

Firmware self-tests remain: S1-U09 (binary codec; replaces retired S1-U04), S2-U03 (Motion policy), S3-U02 (normalization/progress) and S3-U06 (P guidance). S3-U03 remains the retired line-PID case; later P/capture/recovery cases retain their IDs. Network snapshots never count as sensor/watchdog progress.

Actual wireless checks are SYS-09/10 through [wireless setup](../README.md#getting-started). Save Pico-originated subscriber output separately from synthetic/laptop transport evidence. A slow telemetry stream cannot prove short stop/recovery deadlines; use camera/scope measurements.

## Planned integrated trials

Target procedures follow [pending work](../pending.md) and [mission-readiness](mission-readiness.md).
Historical case definitions and results in TEST_LOG files stay as recorded. Differences
such as GP20 reset, JSON output or a dedicated Safety task describe their recorded
source, not target acceptance. Append a new procedure revision/result, never reuse
retired IDs. MR-34 remains retired; MR-40 covers endless operation. S3-M02 remains
physical timing evidence, with final-loop coverage extended through MR-39 rather
than assuming obsolete two-task timing names/limits. SYS-06 watchdog checks await
D15; no enabled watchdog is inferred from startup tick configuration.

All remain pending until actual measurements/results are recorded. This retains the integrated design's IDs without treating proposed checks as passes.

| ID | Scenario / evidence |
| --- | --- |
| T01 | Electrical release: markings, continuity, loaded rails/current, signal highs, shared contacts and jumper |
| T02 | Boot/stop/reset in every motion mode; output-brake timing and physical stopping separately |
| T03 | Straight/curve/junction tracking under lighting, surface and speed variation |
| T04 | Known barcode scales/speeds in both directions, same-character decoding, pending-to-junction execution, duplicate/partial rejection and capture overflow |
| T05 | Four encoder actions: repeated distance/angle accuracy, completion, timeout and obstacle cancellation |
| T06 | Labelled stationary/acceleration/turn/climb/descent/impact traces; no raw gyro claim |
| T07 | Measured reference humps, slip/dynamic/incomplete rejection and invalid estimate after valid maximum |
| T08 | Known forward ranges/materials/angles; no/late/out-of-range echo, settled repeated filtering and repeat timing |
| T09 | Moving scan rejection, coarse/fine profile for 100-200 mm widths, uncertainty, separate front age and restored-center acquisition |
| T10 | Vehicle envelope/course boundary, both bypass sides, blocked paths and original-line versus wrong-crossing recovery |
| T11 | Controlled line loss, interrupted search, obstacle during search and bounded timeout |
| T12 | Actual mission telemetry/status subscription, units/validity and rejection of prohibited/malformed commands |
| T13 | WiFi/broker loss/reconnect under sensor/scan load; healthy local control and no stale replay |
| T14 | Stale/frozen producers, encoder/stall faults, expired command and full diagnostic/navigation/capture buffers |
| T15 | Concurrent load: releases/WCET, missed deadlines, stop latency, queue occupancy and stack/memory usage |
| T16 | Repeated looping-course rehearsal with junction actions, stationary U-turn pause, humps, bypass, telemetry and local-stop ending |

Acceptance targets/unknown values are owned by [project-specs](../docs/project-specs.md#pending-decisions), not inferred from a case name. Demonstration links belong in the [video index](../video/README.md#video-index) and the corresponding result row.

The live completion checklist is [pending work](../pending.md).
Historical plan references in recorded logs refer to their original revisions.

## Qualifying lap records

Use `scripts/mission_trial_score.py` for the [agreed lap](../docs/project-specs.md#demo-and-full-course-decisions-2026-11-03).
Create a CSV under `testing/integration/logs/` with this header (no invented runs):

```csv
trial_id,firmware_sha,uf2_sha256,trace,lighting,success,evidence,notes,lap_complete,barcode_ok,navigation_ok,hump_count_ok,hand_contact,reacquire,stopped_fault,restarted,course_speed_mmps,max_height_error_mm,height_limit_mm
```

Use unique IDs across every image, exact source/image hashes, TRACE 0/1, a stable
lighting label and PASS/FAIL. `evidence` identifies the capture, video/time window
and measurement record; quoted CSV cells may contain a semicolon-separated list.
The eight condition columns use `yes`/`no`; blank is allowed only for failed
attempts whose outcome was not observed. Numeric cells are finite, nonnegative;
failed attempts may leave unmeasured values blank. Failed runs stay in the sample.

Before running, Nicholas records the height error limit in D10. The minimum hump
height of 10 mm is not that limit. For each lap record actual reference heights,
reported heights, crossing and event counts, expected/observed junction actions,
start/end boundary, measured speed evidence and the chosen accuracy limits.
`max_height_error_mm` is the largest absolute reported-versus-reference error;
`height_limit_mm` is the agreed limit, never inferred by the scoring tool.
`course_speed_mmps` is the commanded course setpoint; measured-speed acceptance
must also be supported by the evidence/notes. Retain the separate bypass-speed
scope decision. A programmed pause is allowed; STOPPED/FAULT or restart is not.

The scorer rejects contradictory or incomplete PASS rows, groups by exact image,
TRACE and lighting, and requires at least 19/20 for the 95% sample target. It does
not inspect a video or certify hardware. Do not combine different lighting into
one batch. GP21 after the recorded finish boundary is outside the lap; deliberate
fault/recovery tests use separate result records. Old six-column CSVs must be
supplemented with real evidence, not automatically promoted to this contract.
