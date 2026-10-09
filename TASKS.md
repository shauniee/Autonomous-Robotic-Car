# Project implementation tasks

This file owns current status. Guides own contracts; saved evidence applies only to its recorded source/profile. Full-course requirements remain in [PROJECT_SPECS](docs/PROJECT_SPECS.md).

## Tracking and dashboard integration

Implemented software: independent paired IR acquisition/calibration/P guidance; bounded S3 capture/cancel API; application Mission/Safety coordinator; fresh local start/stop/reset, fixed recovery deadline and watchdog; checked generation-protected S2 output; S1 actual-snapshot collector and display-only localhost dashboard.

Parallel branches: `feat/ir-tracking-movement` (S2), `feat/mqtt-html-dashboard` (S1), `feat/line-tracking-coordinator` (S3/coordinator/shared guides). `feat/line-tracking-integration` combines them in that dependency order. Existing unrelated worktree/untracked files are preserved.

| Validation | Status / evidence owner |
| --- | --- |
| Four sequential firmware profiles | PASS at `37adc44` after the GPIO-address correction; [source manifest](testing/integration/logs/2026-10-09_tracking-r2-source.json) and per-profile build logs/checksums |
| On-target firmware self-tests | PASS: 23/23 at `37adc44`, USB-only wireless TEST; [console](testing/integration/logs/2026-10-09_tracking-r2-pico-test-console.txt) |
| Pico-originated MQTT / dashboard | Pico MQTT PASS for uncalibrated inhibited observations: 350 telemetry frames; [results](testing/integration/logs/2026-10-09_tracking-r2-pico-results.json). Dashboard synthetic software/browser checks PASS; actual Pico live browser rendering remains pending |
| Raised-wheel polarity/brake / output latency | Pending new-image measurement; Nicholas reports existing motor/encoder operation |
| Three straight/curve tracking repetitions | Pending operator physical trials |
| Recovery, GP21, separate reset/start, network loss | Pending recorded trials; use existing system IDs |
| Timing/stack/resource margin | Pending measurement under concurrent load |

Software integration and saved firmware checks are complete. Nicholas stopped further testing on 2026-10-09 because the robot is unavailable; no normal-image flash or physical trials were performed. The last flashed image is motor-inhibited wireless TEST. Completion of the robot acceptance requires recorded new-image physical tracking plus actual live MQTT/dashboard acceptance. [Getting started](docs/GETTING_STARTED.md) owns operation; [testing](testing/README.md) owns stable IDs/evidence. Calibration is RAM-only and must be repeated after flashing/power changes.

## Deferred coursework

Encoder acquisition/PID/distance/angle actions, barcode decoder/navigation, junction/finish interpretation, IMU/terrain, sonar/obstacle and full-course mission remain separate work. Existing S3 ADC/timing/lighting/calibration physical characterization also remains evidence work. No remote motion/control is added.

## S3 consolidation

Implemented on `feat/consolidating-s3`: existing movement/integration consolidated, independent Code 39 capture/decoder/event FIFO, configurable S2 encoder/PID/navigation actions, Mission junction/reacquisition context gates, additive S1/dashboard observations and [S3 learning guide](docs/SUBSYSTEM3_GUIDE.md). Terrain/Obstacle/full-course behavior remains deferred.

Software fixtures and build evidence are recorded under testing. No robot was available: new on-target tests, electrical release, mounted polarity/geometry, PID tuning, physical line/barcode following, turn/stop accuracy and course acceptance remain pending. Required local navigation configuration has no guessed defaults.
