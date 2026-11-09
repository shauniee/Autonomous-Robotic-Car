# Testing and evidence

## Contents

- [Layout](#layout)
- [Renumbered IDs](#renumbered-ids)
- [Continuous integration](#continuous-integration)
- [Run and record](#run-and-record)
- [Preserved software evidence](#preserved-software-evidence)
- [Planned integrated trials](#planned-integrated-trials)

## Layout

A test only counts once it is written here with its result and its saved log.

```
testing/
├── README.md              this guide
├── comms/                 S1, kept by its owner
│   ├── TEST_LOG.md        test cases and results
│   └── logs/              saved console output, photos, scope captures
├── motion/                S2
├── line_barcode/          S3
├── terrain/               S4
├── obstacle/              S5
├── integration/           tests across two or more subsystems (shared)
├── system/                the whole robot against its requirements (shared),
│                          with the mission-readiness checklist
└── host/                  host tests: the real subsystem code on the laptop
                           with fake hardware, run by CI on every PR
```

Choose the folder by what the test exercises, not by where it ran. A barcode
read on the car tests line_barcode only, so it goes in `line_barcode/`. A run
that needs Mission to combine two or more subsystems (line following, junction
turns, bypass) is integration. Builds, layout and timing audits of the whole
image are integration too.

| Level | Stable IDs | Ownership / contract |
| --- | --- | --- |
| Subsystem | `S1-U01`, `S2-U01`, … | Subsystem owner |
| Integration | `INT-01`, … | Both interface owners; [system contracts](../docs/architecture.md#shared-data-rules) and [line and barcode](../app_program/line_barcode/README.md) |
| System | `SYS-01`, …; planned `T01`–`T16` | Team; [requirements](../docs/project-specs.md) |

IDs never change or get reused, including retired template cases, except for the one renumbering below. Test IDs keep their old prefix: S1 is comms, S2 motion, S3 line_barcode, S4 terrain and S5 obstacle. Retired definitions remain in the relevant log; placeholder template rows are not observations.

## Renumbered IDs

On 2026-11-09 the integration results that test only one subsystem moved to that
subsystem's log, with their saved logs, and took a new subsystem ID. Commit
messages and old captures still use the old IDs. The old INT numbers stay retired
and are never reused; the next integration ID is INT-239.

| Folder | Old ID → new ID |
| --- | --- |
| `comms/` | INT-08 → S1-U15, INT-10 → S1-U16, INT-33 → S1-U17, INT-34 → S1-U18, INT-36 → S1-U19, INT-39 → S1-U20, INT-40 → S1-U21, INT-41 → S1-U22, INT-43 → S1-U23, INT-53 → S1-U24, INT-139 → S1-U25, INT-140 → S1-U26 |
| `motion/` | INT-65 → S2-U07, INT-66 → S2-U08, INT-67 → S2-U09, INT-99 → S2-U10, INT-100 → S2-U11, INT-101 → S2-U12, INT-102 → S2-U13, INT-165 → S2-U14, INT-176 → S2-U15, INT-195 → S2-U16 |
| `line_barcode/` | INT-70 → S3-U21, INT-71 → S3-U22, INT-93 → S3-U23, INT-140b → S3-U24, INT-141 → S3-U25, INT-141b → S3-U26, INT-142 → S3-U27, INT-143 → S3-U28, INT-144 → S3-U29, INT-145 → S3-U30, INT-146 → S3-U31, INT-147 → S3-U32, INT-148 → S3-U33, INT-149 → S3-U34, INT-150 → S3-U35, INT-151 → S3-U36, INT-152 → S3-U37, INT-170 → S3-U38, INT-192 → S3-U39, INT-230 → S3-U40, INT-231 → S3-U41, INT-232 → S3-U42, INT-233 → S3-U43, INT-235 → S3-U44, INT-236 → S3-U45, INT-237 → S3-U46 |
| `terrain/` | INT-48 → S4-U09, INT-157 → S4-U10, INT-158 → S4-U11, INT-166 → S4-U12, INT-171 → S4-U13 |
| `obstacle/` | INT-201 → S5-U16 |

## Continuous integration

`.github/workflows/host-tests.yml` runs on every pull request and every push to `main`. None of it runs on the car.

| Job | Command | Fails when |
| --- | --- | --- |
| Coding standard | `python3 scripts/check_barr.py --base origin/main` | A changed line breaks a mechanical BARR-C:2018 rule. `--summary` lists the legacy backlog without failing |
| Deliverables | `python3 scripts/check_deliverables.py` | A subsystem's source, public function, wired-in test or report section is missing; a second motor writer appears; networking commands Motion; firmware and laptop frame lengths differ |
| Host tests | `bash testing/host/run.sh` and `node testing/host/test_dashboard.mjs` | Any test of the real subsystem code fails |
| Firmware build | `sh testing/build_pending_profiles.sh` | Any of the six images fails to compile or link, or a struct outgrows its bound in `testing/sizeof_probe.c`. It never flashes |

Deliverables that need the car print NEEDS EVIDENCE and do not fail the check, because only a physical run can close them.

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
5. Append a result row with seven columns: `Date | ID | Commit | Tester | Result | Evidence | Notes`.
   - **Commit:** the 7-character commit; add `+` for uncommitted changes on top of it. Record the flashed-image checksum in Notes.
   - **Result:** `PASS`, `FAIL`, `PARTIAL`, `OBSERVED` or `PENDING`, then the scope in brackets: `host`, `build`, `static` or `car`. A host or build PASS is not a car PASS.
   - **Notes:** one or two short sentences with the measurements; no repeat of the Result column.
   - Commit the row and its evidence together.
6. Record a failure and its reason; after a fix, append a new result. Keep old rows and measurements. Captures are stored in the compact form `scripts/compact_log.py` writes; gzip any over 1 MB. Do not save build or host output: the result row and commit are enough. Raw captures and build output removed on 2026-11-08 are at git tag `test-logs-raw-2026-11-08`; output from before 2026-11-03 is at `test-logs-full-2026-11-03`. Rerun affected subsystem/integration cases before merging, and the applicable full set before a demonstration.

## Preserved software evidence

Host-only tests for kernel-free modules run with `testing/host/run.sh` (no fake kernel). `run.sh` passes `-DHOST_TEST`, which hides the kernel and driver parts of merged files such as `line.c`, `barcode.c` and `motion.c`. The older 2026-10-08 fake-kernel/hardware/network and laptop-loopback results remain historical software evidence; their recorded methods are not current run instructions. They do not establish GPIO performance, physical motion or an actual Pico connection.

Firmware self-tests remain: S1-U09 (binary codec; replaces retired S1-U04), S2-U03 (Motion policy), S3-U02 (normalization/progress) and S3-U06 (P guidance). S3-U03 remains the retired line-PID case; later P/capture/recovery cases retain their IDs. Network snapshots never count as sensor progress.

Actual wireless checks are SYS-09/10 through [wireless setup](../README.md#getting-started). Save Pico-originated subscriber output separately from synthetic/laptop transport evidence. A slow telemetry stream cannot prove short stop/recovery deadlines; use camera/scope measurements.

## Planned integrated trials

Target procedures follow [open issues](https://github.com/shauniee/Autonomous-Robotic-Car/issues) and [mission-readiness](system/mission-readiness.md).
Historical case definitions and results in TEST_LOG files stay as recorded. Differences
such as GP20 reset, JSON output or a dedicated Safety task describe their recorded
source, not target acceptance. Append a new procedure revision/result, never reuse
retired IDs. MR-34 remains retired; MR-40 covers endless operation. S3-M02 remains
physical timing evidence, with final-loop coverage extended through MR-39 rather
than assuming obsolete two-task timing names/limits. SYS-06 is retired with the
watchdog (D15).

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

The live completion checklist is [open issues](https://github.com/shauniee/Autonomous-Robotic-Car/issues).
Historical plan references in recorded logs refer to their original revisions.
