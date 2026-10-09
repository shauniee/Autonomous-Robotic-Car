# S3 IR service test log

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
| 2026-10-09 | S3-U02/U06/U07/U10/U11/U12/U13 | `37adc44`; [integrated source](../integration/logs/2026-10-09_tracking-r2-source.json) | Nicholas GP20 / Codex USB capture | PASS | [console](../integration/logs/2026-10-09_tracking-r2-pico-test-console.txt), [results](../integration/logs/2026-10-09_tracking-r2-pico-results.json) | Seven firmware checks executed; real cache mutex survived the suite and live S3 acquisition continued during the saved run. U11 is the legacy calibration-helper fixture. No live calibration capture, physical IR characterization or tracking trial was performed. |

## 3. Current firmware run instructions

S3 fixtures and assertions live here in `ir_self_test.c/.h` and `subsystem3_test.c/.h`. `firmware.mk` adds them only with TEST=1 through the shared Make include. Use the `subsystem3_test()` hook already called by `app_main.c`; the private production mutex retains only a TEST bridge, with its assertions defined here. Build profiles and pending integration are recorded in [TASKS](../../TASKS.md#tracking-and-dashboard-integration); the intended calibration/data API is in [Line Tracking](../../docs/architecture/LINE_TRACKING.md).

In TEST builds, S1/S2/S4/S5 tasks start first while the S3 acquisition task remains stopped. Open the serial monitor, release GP20, then press and release GP20 to start the complete firmware self-test suite. The button gate requires about 30 ms stable release/press/release and prints a waiting message every second. S3 acquisition begins only after the suite finishes, so the start press is not treated as a calibration press and its periodic diagnostics are easy to capture. A build is not an executed self-test pass. The millivalues divide by 1000 to obtain the public −1…+1 sensor values; use the printed validity flags.

For a later manual calibration trial, follow the [integrated button/capture policy](../../docs/architecture/LINE_TRACKING.md). Capture black then white and record mounting, lighting, raw ranges, contrasts, cues and timing. GP21 persistently stops and cancels capture; reset and start require separate fresh presses. Reboot replaces calibration. Sensors must remain stationary during capture; S3 does not measure stationarity. Live held/bouncing/simultaneous-button checks remain pending.

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

## Tracking integration control change — 2026-10-09

The S3 acquisition/calibration/guidance algorithms are retained. The application coordinator now interprets runtime buttons and sends bounded captures/cancel; S3 owns GPIO setup, its tracker and buzzer. S3-U11 remains the preserved calibration-helper fixture, not validation of live start/reset policy. New runtime policy/cancellation/watchdog fixtures are INT-11 through INT-14 in [integration](../integration/TEST_LOG.md). All await recorded on-target execution for the integrated source.

## Integrated Pico execution follow-up — 2026-10-09

The `37adc44` wireless TEST run passed all seven S3 fixtures, including S3-U12. The console records snapshot mutex ID 3 before and after the suite and successful task start. Live valid acquisition then continued during the saved 174.5-second telemetry span. This supersedes the earlier pending integrated execution status for this source only; it does not validate physical calibration/tracking or the older unknown-source image. Nicholas stopped further testing because the robot is unavailable.

## S3 consolidation software cases

These fixtures execute the actual pure algorithms with synthetic inputs. Firmware variants compile for TEST=1; new on-Pico execution remains pending. Temporary laptop execution is authorized for this consolidation only and adds no permanent host runner.

| ID | Scope | Method | Expected |
| --- | --- | --- | --- |
| S3-U14 | Code 39 A/B/C/D/Z at two scales | Pure firmware fixture / temporary sanitized laptop harness | All guards and bounded outcomes hold; no hardware claim |
| S3-U15 | Reverse, malformed width/gap and invalid configuration | Pure firmware fixture / temporary sanitized laptop harness | All guards and bounded outcomes hold; no hardware claim |
| S3-U16 | Distance-based events, one-shot rearm and unmapped Z | Pure firmware fixture / temporary sanitized laptop harness | All guards and bounded outcomes hold; no hardware claim |
| S3-U17 | Navigation queue overflow and ambiguous-edge capture loss | Pure firmware fixture / temporary sanitized laptop harness | All guards and bounded outcomes hold; no hardware claim |
| S3-U18 | Encoder/timer wrap and incomplete-frame timeout | Pure firmware fixture / temporary sanitized laptop harness | All guards and bounded outcomes hold; no hardware claim |
| S3-U19 | Actual SPSC ring capacity, order, overflow, purge and wrap | Pure firmware fixture / temporary sanitized laptop harness | All guards and bounded outcomes hold; no hardware claim |

### Consolidation results, 2026-10-09

| Source | Cases | Execution | Result | Evidence |
| --- | --- | --- | --- | --- |
| `48cbb19` | S3-U14–U19 | Temporary laptop harness, actual pure production algorithms, ASan/UBSan | PASS software | [source/compiler manifest](../integration/logs/2026-10-09_consolidating-manifest.json), [fixture output](../integration/logs/2026-10-09_consolidating-software.txt) |
| Same source | S3-U14–U19 | New firmware TEST images on Pico | **NOT EXECUTED** | [nine-profile sequential builds](../integration/logs/2026-10-09_consolidating-acceptance-builds.json); no new board log |

These rows do not validate wiring, ISR ordering/WCET, motor output, mounted sensing, wheel geometry/PID, stopping or original-line recognition. Older on-target rows remain evidence for their recorded revisions only.
