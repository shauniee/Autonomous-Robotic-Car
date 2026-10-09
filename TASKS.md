# Project implementation tasks

This file owns current implementation status and pending integration work. Guides under `docs/` describe intended contracts; historical test records remain evidence of their recorded source/profile, not proof of current behavior. Entries for other owners are coordination tasks, not permission to edit their code.

## S3 line service

### Implemented software

- Independent paired IR acquisition, calibration, classification and proportional guidance in S3.
- GP20 black/white/replacement calibration, GP21 cancellation and asynchronous one/two/three-buzz feedback.
- Public nonblocking `s3_ir_copy()` snapshot with per-channel white −1 / black +1 normalization, references/contrast, validity, original acquisition time and sequence.
- Firmware fixtures/assertions in `testing/subsystem3/`, compiled only for TEST=1 through its S3-only Make include; existing shared startup hooks run them. Periodic TEST IR diagnostics remain in the live task.
- Shared timebase initialization relocated to application startup; S3 contains no S1/S2 calls, motor controller or whole-system watchdog policy.

Offline/wireless TEST and normal profiles built successfully on 2026-10-09, including after the test-source relocation; the firmware self-checks have not been executed on Pico. Build results and pending execution are recorded in [S3 testing](testing/subsystem3/TEST_LOG.md). Builds do not establish sensor/hardware operation.

### Pending integration and validation

| Task | Owner | Completion evidence |
| --- | --- | --- |
| Read `s3_ir_copy()` and map typed fields into telemetry | S1 / Shaun | Preserve validity/age and sequence, skip busy reads, publish actual Pico-originated observations |
| Replace the old S3-fed telemetry adapter | S1 / Shaun | Current S1 still waits for its previous snapshot writer; it receives no new S3 feed after this refactor |
| Consume guidance and request S2 movement | Mission coordinator / S3 coordination with S2 owner | Agreed start/stop permissions, fresh guidance and S2 interface tests; S3 sensor service never enables motors |
| Implement bounded line-loss recovery and original-line search | Mission coordinator with S2 / team | Known-direction and deadline rules, reacquisition confirmation, obstacle/course context and physical trials; do not guess ambiguous crossings |
| Restore system-wide Safety and watchdog policy | Safety coordinator / team | Agreed critical-producer health, independent stop and meaningful progress; no S3 watchdog is currently enabled or fed |
| Coordinate recalibration while the full robot operates | Mission / Safety / S3 | Require braked/stationary calibration, prevent movement during capture, reject stale guidance; the isolated S3 reader does not prove stationarity |
| Reconcile shared documentation | Team / owning guides | Update architecture, TEAM allocations/objects, operating instructions and old IR-to-Motion handoff around the final boundaries; retained shared guides still describe the previous bench |
| Execute S3 firmware checks on Pico | Nicholas | Capture actual `[TEST]` output for the exact image/source checksum |
| Verify ADC protocol and pair timing | Nicholas | Mounted GP26/GP27, discard/average behavior, <=2 ms whole-pair timing, invalid acquisition and bounded fault measurements |
| Verify calibration buttons and buzzer | Nicholas | Held/bouncing/simultaneous inputs, cancellation/replacement, one/two/three-buzz timing and contrast at mounted height |
| Establish line guidance baseline | Nicholas | Reference surfaces/lateral positions, independent channel contrast/polarity, repeatability, lighting, latency and loss/hysteresis |
| Verify concurrency and object/stack budget | Nicholas / S1 | Cross-core reader contention, coherent copies, unchanged acquisition time, freshness under load and measured stack margin |

S2 code is unchanged and retains its boot stop latch with no S3 motor requests or reset calls. Existing TEST inhibition remains. Physical motor operation and full-robot tracking are pending; normal firmware is presently a sensor/calibration service, not a driving bench.

## Barcode

Barcode capture/decoder/navigation remains separate pending S3 work under the [Barcode guide](docs/architecture/BARCODE.md). No comparator polarity, framing, course mapping or encoder context is inferred by the line service.
