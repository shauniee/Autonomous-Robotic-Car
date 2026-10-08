# CSC01 robot project

This repository now follows the instructor's micro T-Kernel template revision `c3b09544cb1fd6dac6ad4b8f523be0af59b44482`. The unchanged upstream [README](README.md) explains the runtime; [getting started](docs/GETTINGS_STARTED.md) explains this robot. [UPSTREAM.json](UPSTREAM.json) records provenance and [TEAM.md](TEAM.md) records ownership/interfaces.

Active firmware: S3 two-sensor proportional IR bench plus S2 motor task, both on core 1. Kernel USB/WiFi services run on core 0. The offline profile uses `NET=none MQTT=0`; the optional wireless profile uses `NET=lwip MQTT=1`. S1 owns a latest telemetry snapshot and one nonblocking mutex, with no application task. S4/S5 remain inactive. Barcode, encoder feedback, terrain, sonar and full mission remain pending.

The wireless profile publishes read-only JSON to `robot/s3/telemetry` every 500 ms. Start Mosquitto and `mosquitto_sub` on the Mac using [these setup instructions](docs/GETTINGS_STARTED.md#5-wireless-telemetry-on-the-mac). Calibration and start/stop remain local buttons; network loss does not change motion. [Communications](docs/architecture/COMMUNICATIONS.md) defines the fields and ownership. Firmware builds and host checks do not establish a Pico connection or physical timing.

This is a later requested S3 bench override: the unchanged Word report describes the earlier PID design. S3 now has READY, FOLLOW_LINE, RECOVER_LINE and STOPPED states. Recovery turns one wheel in the last known direction for <=100 ms, resumes after two fresh visible samples, and stops if direction/data are unavailable. See the [complete IR state catalogue](docs/architecture/LINE_FOLLOWING.md) and [future PID/barcode plan](docs/architecture/IR_FUTURE_PLANS.md).

One power source at a time: USB only for flashing/inhibited TEST=1 checks, disconnected before battery trials. GP20 captures black, captures white and starts on separate fresh presses; GP21 stops/cancels. GP22 chirps 1/2/3 for black/valid white/rejection. Calibration is RAM-only.

IR development template: [Phase 1 IR-to-Motion handoff](docs/architecture/IR_MOTION_HANDOFF.md). The existing request path is available for software development; physical motor operation remains unverified.

Software check: `sh scripts/check-ir-tracking.sh`. Builds: see [getting started](docs/GETTINGS_STARTED.md). Recovery of all pre-migration source/report/figures is in `../recovery/pre-mtkernel-20261008/`; credentials and generated artifacts are excluded.

The configured project repository is private: [Autonomous-Robotic-Car](https://github.com/shauniee/Autonomous-Robotic-Car). Confirm instructor access before hand-in. The saved software checks identify the tested working tree through source manifests; use the resulting Git revision and flashed-image checksum for subsequent on-target trials.
