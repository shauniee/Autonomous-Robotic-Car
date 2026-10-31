# S3 IR service test log

## Contents

- [1. Test cases](#1-test-cases)
- [2. Results](#2-results)
- [3. Current firmware run instructions](#3-current-firmware-run-instructions)
- [4. Sensor-service build verification — 2026-10-09](#4-sensor-service-build-verification--2026-10-09)
- [5. Test-source relocation verification — 2026-10-09](#5-test-source-relocation-verification--2026-10-09)
- [6. Startup test stack correction — 2026-10-09](#6-startup-test-stack-correction--2026-10-09)
- [7. Minimum-bit final memory (S3-M01) — 2026-10-27](#7-minimum-bit-final-memory-s3-m01--2026-10-27)
- [8. Both-black forward (S3-BF01) — 2026-10-28](#8-both-black-forward-s3-bf01--2026-10-28)
- [9. Minimum crossing distance (S3-BF02) — 2026-10-28](#9-minimum-crossing-distance-s3-bf02--2026-10-28)
- [Documentation alignment — 2026-10-29](#documentation-alignment--2026-10-29)
- [Flipped barcode capture — 2026-10-31](#flipped-barcode-capture--2026-10-31)

Owner Nicholas / 2501552. Retired template S3-U01 covered example statistics bounds/minimum/maximum; no template result was recorded and the ID is not reused.

Host methods below describe historical checks whose runners are retired; saved results and firmware self-tests remain.

## 1. Test cases

| ID | Test / requirement | Method / steps | Expected |
|---|---|---|---|
| S3-U02 | Normalization; previous Motion/watchdog progress portion retired | On-target TEST=1 pure fixture | Endpoints, unequal/opposite spans, clamping and 299/300 contrast; no hardware claim |
| S3-U03 | Independent calibration/PID/controller (retired after proportional override; historical results retained) | Host original meaningful IR/control tests | Fresh bounded steering, calibration validity, hysteresis/loss and safety pass |
| S3-U04 | ADC register protocol | Host real drv_ir with fake registers | Left ADC0 then right ADC1; discard first, average 8; error and stalled timer fail boundedly |
| S3-U05 | Local calibration and Motion-health bench policy (retired after sensor-service split) | Historical host test_mtkernel_bench | Fresh black/white/start presses, contrast rejection, GP21 dominance, separate reset/start, stalled progress refuses watchdog feed |
| S3-U06 | Proportional direction, zero error, saturation, polarity and original freshness | Historical host result; current TEST=1 pure fixtures | Bounded memoryless P; 60/61 ms and wraparound; no movement claim |
| S3-U07 | Typed capture outcomes, acquisition validity and line hysteresis | Historical host result; current TEST=1 capture fixtures plus S3-U10 classification | >=500 ms/25 distinct samples; each contrast >=300; failed ADC/capture gap fail closed; wraparound |
| S3-U08 | Observation table and explicit bounded movement recovery (retired after sensor-service split) | Historical host test_robot_control | Nine combinations; both directions; no known direction stops; two distinct qualifying samples; broad black resets streak; >=100 ms stops; stop/reset rules |
| S3-U09 | Future barcode decode/capture | Planned only; define numerical/framing conditions after course confirmation | Complete ordered widths, one event/encounter; rejected frames/overflow explicit; no decoder implemented |
| S3-U10 | Sensor-only observation classification | TEST=1 pure fixtures | Nine combinations; exact 0.15/0.25/0.85 boundaries, LOST hysteresis and nonfinite rejection |
| S3-U11 | Calibration-only local input workflow | TEST=1 synthetic button/time fixtures; physical GP20/21 and buzzer run separately | Held-at-boot, bounce, 30 ms release/press, cancel dominance, white retry, replacement and wraparound; 1/2/3 result counts |
| S3-U12 | Portable snapshot and nonblocking copy | TEST=1 pure fixtures plus actual private mutex boundary | Raw/reference/contrast/normalized coherence; missing/invalid/uncalibrated/capturing/stale flags; unchanged sequence/time on aging; null and locked-copy rejection |
| S3-U13 | ADC destination validation | TEST=1 `drv_ir_read(NULL)` only | Reject null before hardware access; S3-U04 hardware protocol/WCET remains separate |
| S3-M01 | Minimum-bit final memory (spec #100) | Compile-only `testing/subsystem3/sizeof_probe.sh` plus section-4 build command and `arm-none-eabi-size` on the final implementation; S3 `.bss` objects from `nm`/map | Probe totals equal linked S3 objects; final storage bits recorded in the [bit-usage data contract](../../docs/project-specs.md#s3-bit-usage-data-contract-proposed-spec-100-final-evidence-107); no execution claim |
| S3-M02 | Minimum-bit final S3 loop timing (spec #100) | On-target run of the final implementation; method defined before the run (#107) | Worst-case S3 loop time below its configured period, with margin per [D13](../../docs/project-specs.md#pending-decisions); recorded with exact commit; no value assumed |

The current S3 service does not drive motors, feed a whole-system watchdog or push S1 telemetry. Retired cases/results remain historical. Firmware fixtures do not alter the live tracker; the snapshot boundary check briefly locks the real cache and verifies a nonblocking recursive read is refused.

## 2. Results

| Date | ID | Source | Tester | Result | Evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-08 | S3-U03/U04/U05 | Manifest in integration/logs | Codex host run | PASS | ../integration/logs/2026-10-08_host-checks.txt | Host/fake-register evidence only |
| 2026-10-08 | S3-U02 | Not yet run on Pico | Pending operator | PENDING | none | Test firmware built only |
| 2026-10-08 | S3-U04/U05/U06/U07/U08 | ../integration/logs/2026-10-08_s3-proportional-source-sha256.json | Codex host run | PASS | [host log](../integration/logs/2026-10-08_s3-proportional-host-checks.txt) | P/capture/recovery and retained ADC/local/health checks; no physical result |
| 2026-10-08 | S3-U06 | Same source manifest | Pending Pico operator | PENDING | [build evidence](../integration/logs/2026-10-08_s3-proportional-evidence.json) | TEST self-check compiled only; host PASS above is separate |
| 2026-10-08 | S3-U09 | Future implementation | Pending | PENDING | none | Barcode deferred; no decode run |
| 2026-10-09 | S3-U02/U06/U07/U10/U11/U12/U13 | Working tree source SHA-256 manifest in evidence | Pending Pico operator | PENDING | [service build evidence](logs/2026-10-09_s3-line-service_evidence.json) | All four profiles built; firmware checks compiled only, not executed |
| 2026-10-09 | S3-U02/U06/U07/U10/U11/U12/U13 | Relocated test source manifest in evidence | Pending Pico operator | PENDING | [test-layout evidence](logs/2026-10-09_s3-test-layout_evidence.json) | Relocated fixtures compile in all TEST profiles; no on-board execution |
| 2026-10-27 | S3-M01 | Commit `d6ce587` (#107 branch) | Claude Code compile run | PASS | [section 7](#7-minimum-bit-final-memory-s3-m01--2026-10-27) | Compile/link and host only; nothing flashed |
| 2026-10-27 | S3-M02 | Not yet run on Pico | Pending operator (Nicholas) | PENDING | none | On-target final loop timing; tracked in the physical-test PR |

## 3. Current firmware run instructions

S3 fixtures and assertions live here in `ir_self_test.c/.h` and `subsystem3_test.c/.h`. `firmware.mk` adds them only with TEST=1 through the shared Make include. Use the `subsystem3_test()` hook already called by `app_main.c`; the private production mutex retains only a TEST bridge, with its assertions defined here. Build profiles and pending integration are recorded in the realignment tracking issue (#62); the intended calibration/data API is in [Line Tracking](../../docs/architecture/subsystem3.md).

In TEST builds, S1/S2/S4/S5 tasks start first while the S3 acquisition task remains stopped. Open the serial monitor, release GP20, then press and release GP20 to start the complete firmware self-test suite. The button gate requires about 30 ms stable release/press/release and prints a waiting message every second. S3 acquisition begins only after the suite finishes, so the start press is not treated as a calibration press and its periodic diagnostics are easy to capture. A build is not an executed self-test pass. The millivalues divide by 1000 to obtain the public −1…+1 sensor values; use the printed validity flags.

For the manual calibration trial, release/press GP20 over black and hold the pose until one buzz; repeat over white until two buzzes. GP21 cancels and inhibits captures while held; three buzzes indicate cancellation/rejection. Check held/bouncing/simultaneous buttons and replacement captures. Record mounting, lighting, raw ranges, contrasts and timing. Sensors must remain stationary during capture; S3 does not measure stationarity.

Verify ADC pair timing and failure bounds separately under S3-U04. Observe snapshot contention/cross-core timing under S3-U12 on the board after an agreed test reader is connected. Preserve all historical logs; append actual new measurements rather than reclassifying a prior source's PASS.

## 4. Sensor-service build verification — 2026-10-09

All four profiles compiled and linked successfully. Source identity is the working-tree application/Make SHA-256 manifest plus base HEAD in [evidence](logs/2026-10-09_s3-line-service_evidence.json); base HEAD alone is insufficient. The evidence contains exact commands, compiler/SDK information, image checksums and ELF sizes. No firmware was flashed or self-test executed on Pico.

| Profile | Build exit | UF2 SHA-256 | Build log |
| --- | --- | --- | --- |
| offline-test | 0 | `c4de1c0a4fa0be076254d04c0397914b2c687cb458c35b3b71ed12e85fcff1dd` | [log](logs/2026-10-09_s3-line-service_offline-test-build.txt) |
| offline-normal | 0 | `98604dc652501ebde2ed1dea27861f94d63c9467e79cd8f56621bc664eba4bd5` | [log](logs/2026-10-09_s3-line-service_offline-normal-build.txt) |
| wireless-test | 0 | `22992da87ab84916c8b9afd9ede19811f8a49f113154a78b830ab68370701c50` | [log](logs/2026-10-09_s3-line-service_wireless-test-build.txt) |
| wireless-normal | 0 | `07640d690b089999b83c7ed738aab7adf343554cd8ad35ce5abea1621679cf4a` | [log](logs/2026-10-09_s3-line-service_wireless-normal-build.txt) |

Nine S3 C files also passed `-fsyntax-only -Wall -Wextra -Werror` with the wireless TEST profile headers/defines: [strict compiler output](logs/2026-10-09_s3-line-service_strict-compile.txt). Local guide/task/test links, S3 source line lengths, removed dependencies and tracked edit boundaries were checked. Firmware/manual cases remain PENDING until their actual run evidence is captured.

## 5. Test-source relocation verification — 2026-10-09

The S3 fixtures and runner now live in this testing directory. The source build includes `firmware.mk` only as S3 test rules; it selects test objects with TEST=1 and leaves other subsystem sources unchanged. The private snapshot mutex retains a tiny TEST bridge in production source; boundary assertions are here in `subsystem3_test.c`.

All offline/wireless TEST and normal profiles compiled and linked. Symbol inspection confirmed `subsystem3_test` exists in TEST images and is absent from normal images; normal build logs contain no `testing/subsystem3` object compilation/linking. Runtime and test sources also passed strict compiler checks. [Evidence](logs/2026-10-09_s3-test-layout_evidence.json) records current source hashes, commands, image hashes and logs. Previous evidence is preserved; no Pico run is claimed.

## 6. Startup test stack correction — 2026-10-09

Nicholas's pre-fix firmware capture (retained locally, excluded from the PR)
records S3-U12 FAIL: mutex creation/publication/start read ID 2, but the
boundary test later read ID 0. Acquisition initially succeeded, followed by
`[IR] task delay failed` and a core-1 hard fault. The flashed source identity
was not independently established; retain this as operator-provided evidence.

Inspection of the offline TEST ELF found a definite startup stack capacity
defect: `subsystem1_test()` reserves 1268 local bytes plus 36 saved-register
bytes (1304 bytes), before nested calls, on a 1024-byte initial-task stack.
This overflow can corrupt adjacent memory and is consistent with the lost
mutex handle and crash. Nicholas approved increasing the shared initial-task
stack to 4096 bytes for APP_TEST only; normal startup retains 1024 bytes.
No S1/S2 application implementation was changed.

The TEST startup now calls the GP20 release/press/release gate before running
the suite and starts S3 acquisition afterward. This connects the previously
unused gate helper, keeps the start press out of calibration, and removes
acquisition contention during the real snapshot boundary check. Normal
startup ordering is unchanged. TEST mutex/task diagnostics remain available
to establish whether startup IDs survive the suite.

All four profiles were rebuilt sequentially. ELF inspection confirms 4096-byte
startup stacks in TEST images and 1024-byte stacks in normal images. Build
results and UF2 hashes are in [stack-fix evidence](logs/2026-10-09_startup-stack_evidence.json);
individual build logs share its filename prefix. One interrupted overlapping
build attempt was discarded before the sequential rebuilds recorded here.
Post-fix S3-U12 execution and absence of a hard fault remain PENDING until
Nicholas flashes the corrected image and records the full firmware output.

## 7. Minimum-bit final memory (S3-M01) — 2026-10-27

Final implementation after #102-#106 at commit `d6ce587`. Compile and link only; no firmware was executed or flashed. Toolchain arm-none-eabi-gcc 15.3.1 (Arm GNU Toolchain 15.3.Rel1), Pico SDK in `PICO_SDK_PATH`, macOS host.

Section-4 command in `build_make/`: `make clean`, then `make -k -j8 SMP=1 CONSOLE=usb_cdc WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=0`, then `arm-none-eabi-size mtk3pico_smp1_usb_cdc.elf`. Result: 0 warnings, 0 undefined references.

| Build | text | data | bss |
| --- | ---: | ---: | ---: |
| Final implementation (`d6ce587`) | 70464 | 552 | 23756 |

Probe (`testing/subsystem3/sizeof_probe.sh`) against the linked S3 objects (`arm-none-eabi-nm -S` on the S3 object files of the same build):

| Type | `sizeof` (bytes) | Linked object | Object size (bytes) | Instances | Match |
| --- | ---: | --- | ---: | ---: | --- |
| `barcode_edge_t` | 6 | `ring` (`drv_barcode.o`) | 384 | 64 | yes (64 x 6) |
| `barcode_event_t` | 6 | `events` (`barcode.o`) | 48 | 8 | yes (8 x 6) |
| `barcode_frame_t` | 156 | `frame` (`barcode.o`) | 156 | 1 | yes |
| `S3IrSnapshot` | 40 | `latest` (`subsystem3.o`) | 40 | 1 | yes |
| Mission state (`uint8_t`) | 1 | `published_state` (`mission.o`) | 1 | 1 | yes |
| `IrTracker` | 60 | none: S3 task stack | - | 1 | not in `.bss` |

The [bit-usage data contract](../../docs/project-specs.md#s3-bit-usage-data-contract-proposed-spec-100-final-evidence-107) final-bits column matches these sizes. All four firmware profiles also build at this commit (`testing/pure/run.sh` passes, including the new `test_ir_tracking`).

Host results above are separate from firmware results. S3-M02 (on-target loop timing) and the physical line-follow run are not part of this record; they are pending on the robot.


## 8. Both-black forward (S3-BF01) — 2026-10-28

Case: use calibrated, usable BROAD_BLACK inputs with `guidance_valid=false`.
Expect straight FOLLOW at the configured broad speed; preserve sample time;
stop at the forward interval boundary; resume normal tracking on an early
exit; rearm on a new encounter. GP21 and unusable snapshots must interrupt
movement. Check elapsed time across uint32 wrap and retain junction/finish
cases with realistic invalid broad-black guidance.

Result: PASS, host only, `testing/pure/run.sh` exit 0; tester: Codex.
Base commit: `5e6a45b768e7e2c2f8d69ba18c0360b6680d87de`, plus uncommitted
working-tree changes identified by the [source manifest](logs/2026-10-28_S3-BF01_both-black-forward_manifest.json).
All host tests passed; [captured output](logs/2026-10-28_S3-BF01_both-black-forward.txt).
No hardware was used, firmware flashed, or physical movement verified.
The provisional forward interval requires physical calibration (D02/D07).

## 9. Minimum crossing distance (S3-BF02) — 2026-10-28

Supersedes the timed S3-BF01 behavior. Case: usable BROAD_BLACK with invalid
PID guidance must request DISTANCE for 25 mm plus Motion's 3 mm completion
tolerance. Early visible sensing must not shorten the action. Preserve action
ID and acquisition time; accept only matching DONE. Remaining black must
continue straight without another distance action or mission completion.
Check GP21, unusable sensing, failed action, front obstacle and absolute timeout
across timer wrap; retain held-barcode turns after crossing.

Result: PASS, host suite `testing/pure/run.sh` exit 0; tester: Codex.
Wireless normal Docker build (`SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip
MQTT=1 CRASH_REPORT=0 TEST=0 NOGUARDS=0`) also exited 0 and produced the UF2.
Base commit `5e6a45b768e7e2c2f8d69ba18c0360b6680d87de` plus uncommitted changes:
[source and image manifest](logs/2026-10-28_S3-BF02_manifest.json),
[host output](logs/2026-10-28_S3-BF02_crossing-25mm.txt),
[Docker build output](logs/2026-10-28_S3-BF02_docker-build.txt).
No flash or physical distance verification was performed. Geometry/calibration
and a distinct finish criterion remain pending (D02/D03/D07).

## Documentation alignment — 2026-10-29

Planning note only; no test was executed and no historical result is changed.
The [central plan](../../plan.md) owns future implementation sequence;
[mission-readiness](../mission-readiness.md) owns target physical procedures.
Earlier task splits, GP20 reset flows, JSON messages, watchdog assumptions and
configured timing targets describe their recorded revisions. Do not interpret
them as target passes. Use the applicable procedure revision, exact source and
new evidence filenames for reruns; never overwrite evidence or reuse retired IDs.

Link-maintenance note: the obsolete guide destination above now resolves to its
current owning guide; historical case expectations and results are unchanged.

New case S3-U14, defined before the continuation run: thin black patches may
trigger the bounded minimum crossing but must not consume a held barcode based
on subsequent white-surface travel. Qualification requires distinct black
acquisitions and black-surface encoder travel. Straight/unmapped junctions also
need two distinct outgoing acquisitions; stale/repeated snapshots do not rearm.
Expected software behavior only; mounting and junction centre remain D02/D04.

New case S3-U15: all 26 Code 39 payloads in both orientations and two encoder
scales, including count wrap, against the canonical [ZXing Code39Reader table](https://github.com/zxing/zxing/blob/master/core/src/main/java/com/google/zxing/oned/Code39Reader.java).
Expected: same requested A-Z letter, correct orientation, complete framed
capture only. These standard-design fixtures do not verify issued print scales,
mounted sensing, course framing or physical decode reliability.

| Date | ID | Source | Result | Evidence / limits |
| --- | --- | --- | --- | --- |
| 2026-10-29 | S3-U14 / S3-U15 | Base `044889041aac67b78742f3b7004cd545755349fb` plus [working-source manifest](../integration/logs/2026-10-29_INT-16-manifest.json) | PASS (host only) | [Host output](../integration/logs/2026-10-29_INT-16-host.txt); Actual-black qualification, straight outgoing confirmation, A-Z orientation/scale fixtures; software only |


New case S3-U16 (2026-10-30), defined before running: after a 100-count
white approach, a 5-count black blip and a 200-count white gap, feed the
complete forward A fixture. Expect BAD_WIDTH for the discarded false frame,
then DECODED A from the real frame without another quiet zone.

New case S3-U17 (2026-10-30), defined before running: calibrate black=3500,
white=500 on both channels; feed white to enter LOST, fail acquisition, then
feed raw=1100 (0.2 black) and raw=1400 (0.3 black). Expect UNKNOWN/invalid
output during failure, LOST/no guidance at 0.2, and VISIBLE/guidance at 0.3.
Snapshots must preserve the same validity and classification behavior.
Host fixtures only; no wiring, power or hardware calibration used.

| Date | ID | Source | Tester | Result | Evidence / limits |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | S3-U16 / S3-U17 | Base `cc3edb3dd7c92ec76fb2be742171818095fdc49f` plus [working-source manifest](logs/2026-10-30_S3-U16-U17-manifest.json) | Codex | FAIL before fix, PASS after fix (host only) | [Original regression failures](logs/2026-10-30_S3-U16-U17-red-host.txt); [targeted host output](logs/2026-10-30_S3-U16-U17-targeted-host.txt), including 3516 IR replay samples |

The ADC helper consolidation and removal of the read-only ERR write passed
[strict Cortex-M0+ syntax checking](logs/2026-10-30_S3-U16-U17-driver-syntax.txt).
The [full host suite attempt](logs/2026-10-30_S3-U16-U17-host.txt) failed before
S3 compilation because existing Motion edits reference undefined
`MOTION_STALL_GUARD_ENABLED`. Native firmware compilation was unavailable
(`arm-none-eabi-gcc` absent from PATH); Docker socket access was denied in the
sandbox. No linked firmware, ADC hardware protocol or physical pass is claimed.

New cases S3-U18 / S3-U19 (2026-10-30), defined before running: validate minimum-contrast boundaries in both polarities, incomplete/out-of-range references and failed same-colour capture; cancel a white recapture and verify cached samples cannot restore guidance until a new acquisition; verify freshness at and beyond the age limit across timestamp wrap. Existing replay and invalid-acquisition hysteresis cases must still pass. Host-only fixtures; no physical calibration or hardware operation.

| Date | ID | Source | Tester | Result | Evidence / limits |
| --- | --- | --- | --- | --- | --- |
| 2026-10-30 | S3-U18 / S3-U19 | Base `8be851b` plus [source manifest](logs/2026-10-30_S3-U18-U19-manifest.json) | Codex | PASS (host only) | [Build](logs/2026-10-30_S3-U18-U19-build.txt), [test output](logs/2026-10-30_S3-U18-U19-host.txt). MSVC x64 C11, optimized, warnings as errors; 3516 replay samples plus calibration/cancellation/wrap checks and existing invalid-acquisition hysteresis. Docker engine unavailable; no GNU/Arm firmware build or physical pass. |

## Flipped barcode capture — 2026-10-31

S3-U20, defined before running: replay all A-Z as `*X*` in both print
orientations at narrow/wide widths 10/30 and 20/60 encoder counts. Extend
capture checks to demonstrate the missing enable-time reference loses the
first print, and a white reference plus 100 counts of approach decodes it.
Both orientations must retain the same action mapping, tolerate alternating
plus/minus one-count width variation, reject mixed-orientation guards without
changing output destinations, reject a short rearm gap and accept a full one.
Retain existing malformed-width, missed-edge and reversed-wheel rejection tests.
Host fixtures use provisional software limits; no hardware, power, wiring or
measured calibration is involved. Compile the changed driver in an offline
TEST firmware profile; neither compilation nor host decoding establishes GPIO
capture or physical acceptance.

| Date | ID | Source | Tester | Result | Evidence / limits |
| --- | --- | --- | --- | --- | --- |
| 2026-10-31 | S3-U20 | Base `5e63fe0e90776eb834fd3d27ba438a0f8e017d8e` plus [working-source manifest](logs/2026-10-31_S3-U20/manifest.json) | Codex | PASS (host tests and driver compilation); full firmware build FAIL | [Barcode assertions](logs/2026-10-31_S3-U20/barcode-targeted.txt), [full host suite](logs/2026-10-31_S3-U20/host.txt), [Cortex-M0+ driver compile](logs/2026-10-31_S3-U20/driver-compile.txt). [Full firmware attempt](logs/2026-10-31_S3-U20/firmware-build.txt) fails on an unrelated `size_t` conflict through `terrain_evidence.h` in `io_task.c` and `mission.c`. No firmware execution or physical pass. |

Integration continuation 2026-10-31: [INT-49](../integration/TEST_LOG.md#barcode-branch-integration--int-49)
checks the barcode changes with main's audit fixes at exact committed source
`545a6a381b589f033076bd04df825469e2f0b537`. The full host suite,
dashboard/CLI/HTTP regressions and all four README firmware build profiles
pass. This supersedes the earlier full-build failure for this integrated
source only; the failed attempt above remains historical evidence. No
firmware execution or physical barcode acceptance is claimed.
