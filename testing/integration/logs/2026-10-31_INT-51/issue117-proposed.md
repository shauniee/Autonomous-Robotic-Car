## Contents

- [Status](#status)
- [Remaining plan](#remaining-plan)
- [Done when](#done-when)
- [References](#references)

## Status

**Software implemented; physical acceptance incomplete.** Three-task Motion/Mission/I/O, barcode/junction logic, Terrain quality checks, scan/bypass helpers and binary telemetry are on main. INT-49 records host checks and four firmware builds; these are not physical passes.

Bypass remains disabled. The trial build disables stall detection; a reset watchdog is not established. Record actual settings for every test. Networking cannot directly drive/start/stop; GP21 stops/resets after eligible release, and fresh GP20 starts. Healthy operation loops until local stop or a stopping guard.

## Remaining plan

Work in this order; detailed procedures and limits stay in the owning guides below.

- [ ] **Decisions/setup:** obtain fault-byte owner confirmations (#113), agree missing D01–D15 limits, select the exact image/setup, and resolve final stall-guard/watchdog policy. Run MR-25 on a guard-enabled acceptance image; SYS-06 only after the approved watchdog policy is implemented. Fix or justify remaining build warnings.
- [ ] **Electrical/boot — MR-01..11, T01/T02:** continuity, Echo voltage, loaded rails/power, real TEST output, boot inhibition, held-button/fault/reset behavior, mapping/start gates and bounded report-only IMU failure. Measure output braking separately from physical stopping.
- [ ] **Sensor/Motion calibration — MR-12..25:** mounted IR/IMU/sonar/servo, print polarity/scale/quiet zones, encoder signs/counts/edge rate, geometry, wheel/line gains, repeated distance/turn accuracy (including one raised wheel), stopping and stall response.
- [ ] **Line/barcode/junction/recovery — MR-26..28/32:** lighting/laps, first-print and both orientations, runtime mapping/all four actions once at variable-distance junctions, outgoing tracking, overflow/partial rejection, pending-command cancellation and bounded stationary recovery. No fixed advance, turn undo or finish marker.
- [ ] **Humps — MR-29:** measure three references <=30 mm and make three passes each; calibrate height error/thresholds, half-speed/steady measurement and restoration. Reject dynamic/slip/incomplete windows; retain valid count/maximum through stop/new start until reboot.
- [ ] **Obstacle/bypass — MR-30..31:** measure 100–200 mm objects, width/depth/uncertainty and actual chassis/servo coverage. Validate stationary filtering, supported-side choice, fresh forward coverage, rear/full-car clearance and original-line return; stop on unsupported/blocked evidence. Resolve trial-found gaps before explicitly enabling bypass. No blind reverse or curved-course claim.
- [ ] **Network/faults — MR-08/33, T12..T14:** real Pico telemetry/events, malformed/prohibited requests, installed-only ACK, load/reconnect without stale replay, expired/stalled producers and full buffers. Verify local autonomy/stop and record drops.
- [ ] **Timing/resources — MR-39/S3-M02, T15:** agree bounded capture for current tasks; measure TEST and normal-load WCET/jitter/latency, IRQ/sample ages, brake response, ring/stack/heap/peak RAM. Refresh final text/data/bss, target layouts/copies, wire sizes, regressions and four build profiles. Adapt useful PR #115 ideas without restoring obsolete task names or thresholds.
- [ ] **Mixed mission — MR-35..40, T16:** fresh boot/unfamiliar mapping, three consecutive successful sessions on unchanged firmware/constants, varied features and repeated junctions, network interruption and GP21 ending. Reconstruct route order from events/video; a corrective change restarts the sequence. MR-34 stays retired.
- [ ] **Evidence/submission:** append exact commit/profile/image hash, setup, measurements, limits, failures/reruns and actual indexed unlisted videos. Confirm owner approvals and every applicable requirement; refresh the source-pinned completion audit. Static sizes, placeholder videos and host/build passes do not establish robot acceptance.

## Done when

All applicable boxes have linked acceptance evidence, agreed limits/approvals and three unchanged-image successful sessions. Approved scope changes must be linked. Keep this issue **open** while any required work remains.

## References

[Physical procedures](https://github.com/shauniee/Autonomous-Robotic-Car/blob/main/testing/mission-readiness.md) · [requirements/decisions](https://github.com/shauniee/Autonomous-Robotic-Car/blob/main/docs/project-specs.md) · [architecture](https://github.com/shauniee/Autonomous-Robotic-Car/blob/main/docs/architecture.md) · [team/wiring](https://github.com/shauniee/Autonomous-Robotic-Car/blob/main/docs/team.md) · [test records](https://github.com/shauniee/Autonomous-Robotic-Car/blob/main/testing/README.md) · [video index](https://github.com/shauniee/Autonomous-Robotic-Car/blob/main/docs/week13-plan.md#video-index).

Consolidates #129–#133 and unfinished work from older closed issues; #115 is superseded, not merged. Original issue revisions and [pre-consolidation audit](https://github.com/shauniee/Autonomous-Robotic-Car/blob/main/testing/integration/logs/2026-10-31_github-plan-audit/issue-plan-audit.md) retain the history. This issue is the live plan; `plan.md` is retired.
