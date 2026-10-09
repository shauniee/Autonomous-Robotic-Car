# Project implementation tasks

This file owns current status. Guides own contracts; saved evidence applies only to its recorded source/profile. Full-course requirements remain in [PROJECT_SPECS](docs/PROJECT_SPECS.md).

## Tracking and dashboard integration

Implemented software: independent paired IR acquisition/calibration/P guidance; bounded S3 capture/cancel API; application Mission/Safety coordinator; fresh local start/stop/reset, fixed recovery deadline and watchdog; checked generation-protected S2 output; S1 actual-snapshot collector and display-only localhost dashboard.

Parallel branches: `feat/ir-tracking-movement` (S2), `feat/mqtt-html-dashboard` (S1), `feat/line-tracking-coordinator` (S3/coordinator/shared guides). `feat/line-tracking-integration` combines them in that dependency order. Existing unrelated worktree/untracked files are preserved.

| Validation | Status / evidence owner |
| --- | --- |
| Four sequential firmware profiles | To record in integration logs with source and UF2 checksums |
| On-target firmware self-tests | Pending actual TEST run; compilation alone is not a pass |
| Pico-originated MQTT / dashboard | Pending integrated radio/server run |
| Raised-wheel polarity/brake / output latency | Pending new-image measurement; Nicholas reports existing motor/encoder operation |
| Three straight/curve tracking repetitions | Pending operator physical trials |
| Recovery, GP21, separate reset/start, network loss | Pending recorded trials; use existing system IDs |
| Timing/stack/resource margin | Pending measurement under concurrent load |

Completion requires recorded new-image physical tracking plus actual live MQTT/dashboard acceptance. [Getting started](docs/GETTING_STARTED.md) owns operation; [testing](testing/README.md) owns stable IDs/evidence. Calibration is RAM-only and must be repeated after flashing/power changes.

## Deferred coursework

Encoder acquisition/PID/distance/angle actions, barcode decoder/navigation, junction/finish interpretation, IMU/terrain, sonar/obstacle and full-course mission remain separate work. Existing S3 ADC/timing/lighting/calibration physical characterization also remains evidence work. No remote motion/control is added.
