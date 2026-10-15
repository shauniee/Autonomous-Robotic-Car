# Testing

This folder owns all active test code, definitions and evidence. The authorized reset removed old working-tree fixtures/logs, retired host sources and the legacy CSV outside this folder. Earlier history remains in Git; no previous PASS is inherited.

## Definitions and IDs

Five subsystem folders own their firmware fixtures and TEST_LOG; integration owns shared predicates/transport cases; system owns physical T01-T16. Headers/data may supply independent synthetic vectors. IDs keep their meaning and are never reused. New cases use the next unused ID.

Retired/reserved historical IDs: INT-01, INT-03, INT-04, INT-05, INT-06, INT-07, INT-08, INT-09, INT-10, INT-11, INT-14, S1-U01, S1-U02, S1-U03, S1-U05, S1-U08, S2-U01, S2-U02, S2-U03, S2-U04, S2-U05, S3-U01, S3-U02, S3-U03, S3-U04, S3-U05, S3-U07, S3-U08, S3-U10, S3-U11, S3-U13, S4-U01, S4-U02, S5-U01, S5-U02, S5-U03, SYS-01, SYS-02, SYS-03, SYS-04, SYS-05, SYS-06, SYS-07, SYS-08, SYS-09, SYS-10. These are not current run instructions. T01-T16 remain the physical scenario IDs; SYS-01-SYS-10 are reserved historical references.

## Run and record

Define steps, expected result and agreed numerical thresholds before execution. Use [GUIDE](../GUIDE.md) for setup, power, calibration and operating steps. USB alone for inhibited firmware; battery alone for normal movement. Start one console/subscriber capture before the run.

Firmware checks print `[TEST] ID PASS|FAIL`. Save complete output under the owning `logs/` directory. A build is compilation evidence; an executed firmware fixture is logic evidence; a powered hardware/mission run is physical evidence. Keep these result categories separate.

Each new result records date/IDs, committed revision, profile/UF2 SHA-256, tester, wiring/power, configuration/calibration, mapping request/revision, reference and observed measurements/errors, elapsed time and console/MQTT/video paths. Append failures and reruns; do not overwrite new evidence. No Pico run is claimed without real device output.

## Five practical passes

| Pass | Actions / expected outcome |
| --- | --- |
| 1 Software | Four sequential offline/wireless TEST/normal builds; changed-code strict checks, normal fixture exclusion, source/image identity, payload/resource bounds, links/capitalization and unchanged main/video |
| 2 USB inhibited | Execute all fixtures and capture actual PASS/FAIL; motors remain braked; inspect raw sensor changes |
| 3 Raised wheels/calibration | Electrical release, signed encoder/motor feedback, geometry/PID/accuracy/braking, combined black+IMU calibration, contrast and sonar/servo measurements |
| 4 Feature course | T03-T11: all alphabet/scales/orientations, four actions, short loss, reference/one-wheel humps, both bypass sides and original route |
| 5 Faults/rehearsal | T02/T12-T16: stale sensing, encoder/stall/deadline/buffer/network faults, stop/reset and repeated complete courses with different mappings on identical firmware |

Keep tests focused: existing Make profiles, firmware checks, one capture and concise measurements. Do not introduce a host fake-kernel framework. Synthetic fixture_config.h is never mission configuration. Numerical pass marks and unconfirmed course rules remain in [PROJECT_SPECS](../docs/donotsubmit/PROJECT_SPECS.md#pending-decisions).

A-Z vectors are independent standard-pattern positions checked against [ZXing's primary Code39Reader](https://github.com/zxing/zxing/blob/master/core/src/main/java/com/google/zxing/oned/Code39Reader.java). The 104 width-vector combinations are not physical print/GPIO acceptance or confirmation of the issued symbology.

## Current evidence

[Integration results](integration/TEST_LOG.md) record current compiler/build/helper evidence. Actual USB-only firmware execution and partial sensor/MQTT observations are now recorded below; full physical acceptance remains pending. [System definitions](system/TEST_LOG.md) own the actual scenarios; [videos](../videos/README.md) owns demonstration links.

The baseline software check on 2026-10-15 built all four profiles at `c5e1832`, excluded fixtures from normal images and passed 54 strict target-compiler checks. Its 37 firmware check IDs compiled with execution PENDING. Portable JSON encoding and helper input validation executed on the laptop; these do not establish MQTT delivery or robot behavior. [Baseline evidence index](integration/logs/2026-10-15_fresh-acceptance_evidence.json) preserves those identities and remaining gates.

Continued mission hardening at `c204e39` built all four profiles, passed 62 strict target checks and five static-analysis checks, and compiled 42 firmware IDs including a real-kernel priority-ceiling fixture. These firmware checks still require execution on the Pico. [Current integration results](integration/TEST_LOG.md) and [readiness audit](integration/logs/2026-10-15_mission-readiness.json) retain the distinction between software checks and mission/submission acceptance. Video is excluded from that readiness decision; missing calibration/hardware/physical evidence still prevents release.

The available-hardware continuation executed all 45 current firmware checks on the Pico at `61bc484`, captured IR calibration/static states and slow manual encoder trials, and verified actual stopped MQTT replacement/rejection. The fast encoder trial failure is retained. `33ea475` passed six build profiles and 96 strict object checks; its on-target follow-up is recorded in [integration results](integration/TEST_LOG.md). [Current electrical status](subsystem2/logs/2026-10-15_electrical-status.json) prevents powered tuning until motor/supply ratings are verified. Historical pending statements above refer to their dated revisions.

Latest on-target follow-up at `33ea475`: all 45 firmware checks PASS; 12 actual MQTT samples remain within 60 ms freshness, raw encoder validity remains true, two different four-letter tables receive live acknowledgements on the same image. See [S1 results](subsystem1/TEST_LOG.md) and [current readiness](integration/logs/2026-10-15_available-readiness.json). No powered course acceptance is inferred.
