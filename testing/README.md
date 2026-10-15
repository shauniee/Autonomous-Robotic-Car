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

[Integration results](integration/TEST_LOG.md) record current compiler/build/helper evidence. Firmware execution and physical passes remain PENDING until the board/operator supplies observations. [System definitions](system/TEST_LOG.md) own the actual scenarios; [videos](../videos/README.md) owns demonstration links.

The final software check on 2026-10-15 built all four profiles at `c5e1832`, excluded fixtures from normal images and passed 54 strict target-compiler checks. All 37 firmware check IDs are compiled, with execution PENDING. Portable JSON encoding and helper input validation executed on the laptop; these do not establish MQTT delivery or robot behavior. [Evidence index](integration/logs/2026-10-15_fresh-acceptance_evidence.json) records identities and remaining gates.
