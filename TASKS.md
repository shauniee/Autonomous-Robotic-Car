# Project implementation tasks

This file owns current implementation status and pending integration work. Guides under `docs/` describe intended contracts; historical test records remain evidence of their recorded source/profile, not proof of current behavior. Entries for other owners are coordination tasks, not permission to edit their code.

## S3 line service

### Implemented software

- Independent paired IR acquisition, calibration, classification and proportional guidance in S3.
- GP20 black/white calibration (reboot to replace a completed pair), GP21 cancellation and asynchronous one/two/three-buzz feedback.
- Public nonblocking `s3_ir_copy()` snapshot with per-channel white −1 / black +1 normalization, references/contrast, validity, original acquisition time and sequence.
- Firmware fixtures/assertions in `testing/subsystem3/`, compiled only for TEST=1 through its S3-only Make include; existing shared startup hooks run them. Periodic TEST IR diagnostics remain in the live task.
- Shared timebase initialization relocated to application startup; S3 contains no S1/S2 calls, motor controller or whole-system watchdog policy.

Offline/wireless TEST and normal profiles built successfully on 2026-10-09, including after the test-source relocation; the firmware self-checks have not been executed on Pico. Build results and pending execution are recorded in [S3 testing](testing/subsystem3/TEST_LOG.md). Builds do not establish sensor/hardware operation.

### Pending integration and validation

| Task | Owner | Completion evidence |
| --- | --- | --- |
| Read `s3_ir_copy()` and map typed fields into telemetry | S1 / Shaun | Preserve validity/age and sequence, skip busy reads, publish actual Pico-originated observations |
| Replace the old S3-fed telemetry adapter | S1 / Shaun | Current S1 still waits for its previous snapshot writer; it receives no new S3 feed after this refactor |
| Physically validate application snapshot-to-Motion controller | Nicholas / S2 owner | GP20 start, GP21 stop, acknowledged reset, wheel polarity and straight/curve tracking; implemented policy and firmware fixtures are software only |
| Validate 500 ms bench recovery; design full original-line search separately | Nicholas / team | Bench has last-direction pivot and two fresh visible acquisitions; physical trials and full obstacle/course context remain pending |
| Restore system-wide Safety and watchdog policy | Safety coordinator / team | Agreed critical-producer health, independent stop and meaningful progress; no S3 watchdog is currently enabled or fed |
| Coordinate recalibration while the full robot operates | Mission / Safety / S3 | Require braked/stationary calibration, prevent movement during capture, reject stale guidance; the isolated S3 reader does not prove stationarity |
| Verify application task scheduling and stop response | Nicholas / S2 owner | Core-1 priority 14, 10 ms controller release; measure contention, missed deadlines and physical stop response |
| Execute S3 firmware checks on Pico | Nicholas | Capture actual `[TEST]` output for the exact image/source checksum |
| Verify ADC protocol and pair timing | Nicholas | Mounted GP26/GP27, discard/average behavior, <=2 ms whole-pair timing, invalid acquisition and bounded fault measurements |
| Verify calibration buttons and buzzer | Nicholas | Held/bouncing/simultaneous inputs, cancellation/retained calibration, one/two/three-buzz timing and contrast at mounted height |
| Establish line guidance baseline | Nicholas | Reference surfaces/lateral positions, independent channel contrast/polarity, repeatability, lighting, latency and loss/hysteresis |
| Verify concurrency and object/stack budget | Nicholas / S1 | Cross-core reader contention, coherent copies, unchanged acquisition time, freshness under load and measured stack margin |

S2 code is unchanged and retains its boot stop latch and TEST inhibition. The application controller now reads S3 snapshots and uses S2 request/stop/reset APIs. It owns fresh start, separate reset/restart, duty mixing and user-selected 500 ms bounded recovery. S3 itself has no motor calls. Physical motor operation, firmware fixture execution and full-robot tracking remain pending.

## Barcode

S3 now contains bounded digital-edge buffering, a provisional single-letter Code 39 A–Z decoder in both orientations, measured encoder-width processing, local mapping replacement, nonblocking status and an eight-event copied FIFO. The existing S3 task services a fixed edge budget; barcode adds one private mutex and no task. See the [Barcode guide](docs/architecture/BARCODE.md) for configuration and interfaces.

The scanner boots disabled. Physical capture still requires mounted polarity/width/quiet-zone measurements, a validated S2 encoder provider and agreed shared GPIO dispatch. All letters start unmapped. S1 Wi-Fi mapping configuration, Mission junction handling and S2 navigation actions remain separate owner integration work; no motor/network implementation was changed for barcode.

Nicholas requested preserving the S3 test removal and adding no testing yet. No new test code, on-target barcode result or physical scan is claimed. Compilation/static review is recorded separately from future scanning acceptance.
