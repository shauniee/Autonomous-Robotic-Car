# GitHub issues and implementation-plan audit

## Contents

- [Scope and result](#scope-and-result)
- [Open-issue decisions](#open-issue-decisions)
- [Every GitHub issue](#every-github-issue)
- [Plan stages](#plan-stages)
- [Documentation and acceptance gaps](#documentation-and-acceptance-gaps)
- [Recommended next actions](#recommended-next-actions)
- [Verification and limits](#verification-and-limits)

## Scope and result

Read all **56 issues: 6 open, 50 closed**, including available issue comments
and closure records, on 2026-10-31. Inspected current main at
`565bae7e95668d172fe4b632a14209afc4b4b548`. GitHub state is a snapshot, not a completion verdict.
No GitHub issue, PR, code, plan or existing result was changed. This report
and the retrieved issue snapshots are local, uncommitted audit artifacts.

The core three-task refactor and most software features are implemented.
The full plan is **not complete**: physical calibration/acceptance is missing,
bypass is disabled, and documentation/checker evidence is stale. Several
closed physical-test issues were closed by cleanup, not by test completion.

## Open-issue decisions

| Issue | Current assessment | Suggested disposition, not performed |
| --- | --- | --- |
| [129](https://github.com/shauniee/Autonomous-Robotic-Car/issues/129) | Most contracts exist; plan/checklist contradictions remain | Finish documentation reconciliation, then review for closure |
| [130](https://github.com/shauniee/Autonomous-Robotic-Car/issues/130) | Stop-path refactor implemented and software-tested | Candidate for software-scope closure after review; retain physical checks in 117 and explicitly acknowledge disabled trial stall guard |
| [131](https://github.com/shauniee/Autonomous-Robotic-Car/issues/131) | Mission/line/barcode/obstacle consolidation implemented | Candidate for software-scope closure after review; do not claim physical bypass/navigation acceptance |
| [132](https://github.com/shauniee/Autonomous-Robotic-Car/issues/132) | Bounded I/O and sharing refactor implemented | Candidate for software-scope closure after review; keep real load/freshness/stack checks in 117/133 |
| [133](https://github.com/shauniee/Autonomous-Robotic-Car/issues/133) | Host/build integration done; physical/timing scope incomplete | Keep open or explicitly split remaining acceptance into 117 without losing its requirements |
| [117](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) | Physical readiness incomplete; instructions also stale | Keep open, reconcile procedures before executing them |

## Every GitHub issue

“Implemented in software” means source/tests exist for the current design; it
does not certify every original checkbox, owner sign-off or physical behavior.
“Superseded” identifies a changed requirement rather than missing code.

| Issue / title | GitHub state | Assessment | Evidence and remaining work |
| --- | --- | --- | --- |
| [#21: chore/build-hygiene](https://github.com/shauniee/Autonomous-Robotic-Car/issues/21) | CLOSED | Partial / superseded tooling | Build flags and profile builds exist, but scripts/build.sh has been replaced by Make/Docker. Current build logs still contain the known upstream lwIP unused-function warning; the original zero-warning criterion is not literally met. [build_make/pico_rp2040.mk](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/build_make/pico_rp2040.mk) |
| [#22: test/cr-baseline-before](https://github.com/shauniee/Autonomous-Robotic-Car/issues/22) | CLOSED | Historical evidence; comparison superseded | Pinned baseline measurements are recorded. The course update explicitly removes the before/after requirement; do not redo the old requested report merely because its original path changed. [docs/project-specs.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/docs/project-specs.md) |
| [#23: refactor/timebase-wrap-safe-time](https://github.com/shauniee/Autonomous-Robotic-Car/issues/23) | CLOSED | Implemented in software | Shared ms/us helpers, wrap-safe arithmetic and fixed-rate releases; pure wrap tests exist. Physical tick/stop-latency acceptance remains separate. [app_program/common/robot_time_math.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/common/robot_time_math.h) |
| [#24: refactor/gpio-irq-dispatcher](https://github.com/shauniee/Autonomous-Robotic-Car/issues/24) | CLOSED | Implemented in software | One core-1 GPIO dispatcher serves encoder, barcode and Echo handlers. Maximum physical IRQ load remains unmeasured. [app_program/common/drv_gpio_irq.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/common/drv_gpio_irq.c) |
| [#25: fix/startup-safety-inhibit](https://github.com/shauniee/Autonomous-Robotic-Car/issues/25) | CLOSED | Implemented in software | Required startup failure inhibits output; TEST remains inhibited; sonar acquisition is stepped. Init-failure hardware acceptance is not implied. [app_program/app_main.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/app_main.c) |
| [#26: refactor/config-defines-static-asserts](https://github.com/shauniee/Autonomous-Robotic-Car/issues/26) | CLOSED | Implemented in software | Central tunables, provisional decision references and static relation checks exist. [app_program/common/robot_config.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/common/robot_config.h) |
| [#27: feat/packed-data-layer](https://github.com/shauniee/Autonomous-Robotic-Car/issues/27) | CLOSED | Implemented in software | Atomic word publication, bounded larger copies, owner fault bytes and consistency stress tests exist. [app_program/common/robot_data.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/common/robot_data.h) |
| [#28: docs/data-contract-review](https://github.com/shauniee/Autonomous-Robotic-Car/issues/28) | CLOSED | Partial: approvals remain | Data rows exist, but provisional widths/calibration and owner confirmation have not all been resolved. Closure is not evidence of joint approval. [docs/project-specs.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/docs/project-specs.md) |
| [#29: refactor/encoder-driver-wrap-safe](https://github.com/shauniee/Autonomous-Robotic-Car/issues/29) | CLOSED | Implemented; calibration pending | Quadrature capture, modular per-consumer travel and wrap tests exist; measured count/geometry/edge-rate justification remains D03. [app_program/subsystem2/encoder_math.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem2/encoder_math.h) |
| [#30: refactor/bidirectional-motor-driver](https://github.com/shauniee/Autonomous-Robotic-Car/issues/30) | CLOSED | Implemented; physical acceptance pending | Signed, clamped motor output and per-wheel polarity constants exist; both-direction car trials are not established by the closed issue. [app_program/subsystem2/drv_motor.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem2/drv_motor.c) |
| [#31: refactor/closed-loop-motion](https://github.com/shauniee/Autonomous-Robotic-Car/issues/31) | CLOSED | Implemented in software | Wheel PID, distance/turn actions, IDs, expiry and stop service exist. Stall guard code is present but disabled by the documented current trial setting. [app_program/subsystem2/motion_control.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem2/motion_control.c) |
| [#32: test/motion-calibration-pid-tuning](https://github.com/shauniee/Autonomous-Robotic-Car/issues/32) | CLOSED | Not physically completed | No current accepted raw calibration/stop-latency report; D03/D07/D08 remain open. Closed during cleanup, not on measured completion. [testing/subsystem2/TEST_LOG.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/testing/subsystem2/TEST_LOG.md) |
| [#33: fix/ir-adc-averaging](https://github.com/shauniee/Autonomous-Robotic-Car/issues/33) | CLOSED | Implemented in software | ADC averaging uses the configured exact sample count/divisor. Mounted ADC behavior still needs real-sensor evidence. [app_program/subsystem3/drv_ir_tracking.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem3/drv_ir_tracking.c) |
| [#34: fix/line-follow-steering-saturation](https://github.com/shauniee/Autonomous-Robotic-Car/issues/34) | CLOSED | Implemented; old duty path superseded | Static bounds exist; current Mission requests speed/steering with curve slowdown and peak wheel-speed checks rather than the obsolete direct-duty follower. [app_program/mission/mission_logic.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/mission/mission_logic.c) |
| [#35: refactor/barcode-stack-collapse](https://github.com/shauniee/Autonomous-Robotic-Car/issues/35) | CLOSED | Implemented in software | Bounded edge ring, one service, Code 39 A–Z both orientations, mapping and overflow faults. Latest first-print capture fix is on main via PR 141. [app_program/subsystem3/barcode.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem3/barcode.c) |
| [#36: refactor/mission-core-state-machine](https://github.com/shauniee/Autonomous-Robotic-Car/issues/36) | CLOSED | Implemented under revised design | Mission owns the seven-state repeating loop and pending junction actions. CALIBRATE/COMPLETE and finish detection from this old issue are intentionally superseded. [app_program/mission/mission_logic.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/mission/mission_logic.c) |
| [#37: refactor/safety-task-stop-latch](https://github.com/shauniee/Autonomous-Robotic-Car/issues/37) | CLOSED | Superseded by 130 | Separate Safety task removed; small policy functions and Motion enforce stops. GP21 release resets and fresh GP20 starts; the old GP20-reset requirement is obsolete. [app_program/mission/safety.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/mission/safety.c) |
| [#38: feat/mission-bypass-side-watch](https://github.com/shauniee/Autonomous-Robotic-Car/issues/38) | CLOSED | Partial / disabled for normal use | Bounded side-watch, depth, rear-clearance and original-line helpers exist and have synthetic tests; MISSION_BYPASS_RELEASED=0 and collision-free physical acceptance is missing. [app_program/mission/bypass_control.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/mission/bypass_control.c) |
| [#39: feat/binary-mqtt-schema-v1](https://github.com/shauniee/Autonomous-Robotic-Car/issues/39) | CLOSED | Implemented; schema v1 superseded | Explicit little-endian schema v2, compact messages, reserved-bit validation and shared golden vectors exist; owning protocol is architecture.md. [app_program/subsystem1/mqtt_frame.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem1/mqtt_frame.c) |
| [#40: refactor/mqtt-opaque-frame-adapter](https://github.com/shauniee/Autonomous-Robotic-Car/issues/40) | CLOSED | Implemented; live reconnect pending | Opaque transport, bounded bridge, reconnect/status/will and Network-only fault updates exist. Actual Pico reconnect under load remains 117. [lib/liblwip/sysdepend/pico_rp2040/mqtt_utk.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/lib/liblwip/sysdepend/pico_rp2040/mqtt_utk.c) |
| [#41: feat/s1-reporting-task-event-log](https://github.com/shauniee/Autonomous-Robotic-Car/issues/41) | CLOSED | Implemented in revised task layout | Bounded core-0 reporting, packed events and installed-only mapping ACK exist; no separate Reporting task is required now. [app_program/subsystem1/subsystem1.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem1/subsystem1.c) |
| [#42: chore/pc-mqtt-client-binary](https://github.com/shauniee/Autonomous-Robotic-Car/issues/42) | CLOSED | Implemented in software | Readable binary monitor, mapping, snapshots, malformed telemetry/event rejection and CLI exit-status regressions exist. [scripts/mqtt_client.py](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/scripts/mqtt_client.py) |
| [#43: test/telemetry-concurrency-test](https://github.com/shauniee/Autonomous-Robotic-Car/issues/43) | CLOSED | Implemented as host test | Stress test asserts read/frame consistency with explicit iteration counts; this does not establish RTOS timing or hardware atomicity margins. [testing/pure/test_s1_stress.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/testing/pure/test_s1_stress.c) |
| [#44: fix/s4-impact-latch-invalid-data](https://github.com/shauniee/Autonomous-Robotic-Car/issues/44) | CLOSED | Implemented in software | Impact remains pending until serviced braking plus a fresh clear sample; acquisition/calibration validity is separate. [app_program/subsystem4/terrain_evidence.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem4/terrain_evidence.h) |
| [#45: refactor/gy511-robustness-fixed-rate](https://github.com/shauniee/Autonomous-Robotic-Car/issues/45) | CLOSED | Implemented in software | Bounded whole-operation I2C timeout, fresh/pending/error results, due times and IMU fault reporting exist; physical bus recovery/timing is pending. [app_program/subsystem4/drv_gy511.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem4/drv_gy511.c) |
| [#46: feat/hump-measurement-height](https://github.com/shauniee/Autonomous-Robotic-Car/issues/46) | CLOSED | Implemented; calibration pending | Filtered-pitch times measured travel, complete-window count/max and packed results exist. PR 140 adds acceleration/motion quality rejection; height accuracy still needs references. [app_program/subsystem4/terrain_algo.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem4/terrain_algo.c) |
| [#47: test/hump-reference-trials](https://github.com/shauniee/Autonomous-Robotic-Car/issues/47) | CLOSED | Not physically completed | No accepted raw reference-hump/error report; reference heights, error target and calibrated thresholds remain D06 / 117. [testing/subsystem4/TEST_LOG.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/testing/subsystem4/TEST_LOG.md) |
| [#48: refactor/echo-capture-dispatcher](https://github.com/shauniee/Autonomous-Robotic-Car/issues/48) | CLOSED | Implemented; electrical evidence limited | Echo uses dispatcher capture and bounded trigger/timeout steps. User confirmed electrical behavior; this is not a saved numeric Echo-voltage measurement. [app_program/subsystem5/subsystem5.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem5/subsystem5.c) |
| [#49: feat/servo-scan-obstacle-profile](https://github.com/shauniee/Autonomous-Robotic-Car/issues/49) | CLOSED | Implemented software; acceptance pending | Stationary coarse/fine scan, bounded median groups, qualified edges, profile/clearance and side-watch exist. Physical accuracy/coverage is not accepted. [app_program/subsystem5/profile_edges.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem5/profile_edges.c) |
| [#50: feat/min-bit-s1](https://github.com/shauniee/Autonomous-Robotic-Car/issues/50) | CLOSED | Implemented; detailed follow-up 112 | Packed S1 storage, explicit binary fields and boundary/overflow tests are present. [app_program/subsystem1/s1_report.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem1/s1_report.h) |
| [#51: feat/min-bit-s2](https://github.com/shauniee/Autonomous-Robotic-Car/issues/51) | CLOSED | Implemented; detailed follow-up 109 | Packed command/health and Q7 steering with round-trip tests and ARM size evidence are present. [app_program/subsystem2/motion_control.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem2/motion_control.h) |
| [#52: feat/min-bit-s3](https://github.com/shauniee/Autonomous-Robotic-Car/issues/52) | CLOSED | Implemented in software | S3 snapshots/capture/events and Mission histories have documented narrowed layouts and fixtures; actual sensor calibration is separate. [docs/project-specs.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/docs/project-specs.md) |
| [#53: feat/min-bit-s4](https://github.com/shauniee/Autonomous-Robotic-Car/issues/53) | CLOSED | Implemented; detailed follow-up 110 | Persistent filter/calibration/height/state use scaled integers and packed flags. [app_program/subsystem4/terrain_algo.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem4/terrain_algo.h) |
| [#54: feat/min-bit-s5](https://github.com/shauniee/Autonomous-Robotic-Car/issues/54) | CLOSED | Implemented; detailed follow-up 111 | Packed profile/rays/angle representations and boundary fixtures exist; later sweep extent is +/-20 degrees. [app_program/subsystem5/obstacle_profile.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem5/obstacle_profile.h) |
| [#55: docs/week13-evidence](https://github.com/shauniee/Autonomous-Robotic-Car/issues/55) | CLOSED | Partial submission evidence | Static RAM/message/probe evidence exists, with newer refreshes. Final live timing, packing overhead/stack margins and physical no-regression evidence are incomplete; old before/after comparison is no longer required. [docs/week13-plan.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/docs/week13-plan.md) |
| [#56: test/host-test-runner](https://github.com/shauniee/Autonomous-Robotic-Car/issues/56) | CLOSED | Implemented in software | Kernel-free pure runner and fixtures are active. Current exact-source host results are in INT-49. [testing/pure/run.sh](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/testing/pure/run.sh) |
| [#57: test/on-target-tests](https://github.com/shauniee/Autonomous-Robotic-Car/issues/57) | CLOSED | Test scaffolding implemented; execution pending | APP_TEST-gated subsystem self-tests and startup calls exist on main now, despite old cleanup comments saying they were not merged. No current on-target PASS follows from compilation. [app_program/app_main.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/app_main.c) |
| [#58: docs/docs-cleanup](https://github.com/shauniee/Autonomous-Robotic-Car/issues/58) | CLOSED | Partial documentation reconciliation | Single owning guides/lowercase docs exist, but plan.md, issue 117 and readiness procedures still contain contradictions; original acceptance is not fully current. [plan.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/plan.md) |
| [#59: test/s3-physical-ir-barcode](https://github.com/shauniee/Autonomous-Robotic-Car/issues/59) | CLOSED | Not physically completed | Both-orientation software fixtures and capture fix exist; issued-print contrast, lighting, polarity and junction-offset measurements remain D01/D04. [testing/subsystem3/TEST_LOG.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/testing/subsystem3/TEST_LOG.md) |
| [#60: test/s5-physical-echo-servo](https://github.com/shauniee/Autonomous-Robotic-Car/issues/60) | CLOSED | Not physically completed | User electrical confirmation is recorded; range error, servo endpoints/settling and D05 calibration evidence remain missing. [testing/subsystem5/TEST_LOG.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/testing/subsystem5/TEST_LOG.md) |
| [#61: test/integrated-course-trials](https://github.com/shauniee/Autonomous-Robotic-Car/issues/61) | CLOSED | Not physically completed | No three unchanged-image accepted course sessions with actual timings/videos. Work belongs under 117 / Stage 9. [testing/system/TEST_LOG.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/testing/system/TEST_LOG.md) |
| [#62: chore/realign-main-tracking](https://github.com/shauniee/Autonomous-Robotic-Car/issues/62) | CLOSED | Historical umbrella; full outcome incomplete | Stacked foundation/features landed, but cleanup closure does not establish M1–M10 physical acceptance or final submission completeness. [plan.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/plan.md) |
| [#109: S2 minimum-bit: MotionCommand and MotionHealth](https://github.com/shauniee/Autonomous-Robotic-Car/issues/109) | CLOSED | Implemented in software | MotionCommand and MotionHealth are 12 bytes with accessors, integer steering and target/field validation; tests and target probes exist. [app_program/subsystem2/motion_control.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem2/motion_control.h) |
| [#110: S4 minimum-bit: TerrainContext](https://github.com/shauniee/Autonomous-Robotic-Car/issues/110) | CLOSED | Implemented in software | Q16 filter, packed calibration sums, scaled pitch and float-reference replay exist; TerrainContext stays 32 bytes after the quality bit. [app_program/subsystem4/terrain_algo.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem4/terrain_algo.h) |
| [#111: S5 minimum-bit: ObstacleProfile, ObstacleRay and side-watch angle](https://github.com/shauniee/Autonomous-Robotic-Car/issues/111) | CLOSED | Implemented under updated angle bounds | ObstacleProfile is 14 bytes and ray is 4; current +/-20-degree sweep supersedes the old +/-60-degree request, with documented storage/tests. [app_program/subsystem5/obstacle_profile.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem5/obstacle_profile.h) |
| [#112: S1 minimum-bit: event log entries and event frame](https://github.com/shauniee/Autonomous-Robotic-Car/issues/112) | CLOSED | Implemented in software | 10-byte packed event entries and 168-byte ring; 11-byte schema-v2 wire event retained because obstacle depth needs 16 bits. [app_program/subsystem1/s1_report.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem1/s1_report.h) |
| [#113: Fault bitmask: finalise PROVISIONAL bit positions](https://github.com/shauniee/Autonomous-Robotic-Car/issues/113) | CLOSED | Not fully accepted despite closure | Fault layout agrees in code/tests, but robot_data.h still explicitly awaits owner confirmation. Comments request sign-off; no recorded confirmations were found in the retrieved issue discussion. [app_program/common/robot_data.h](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/common/robot_data.h) |
| [#114: Week 13 whole-system final evidence: RAM and message sizes](https://github.com/shauniee/Autonomous-Robotic-Car/issues/114) | CLOSED | Historical static evidence complete; refresh final submission | SYS-11 records pinned text/data/bss, sizes and wire lengths; SYS-12 and later builds are newer. It does not establish current peak RAM/stack headroom or a final all-object census after later fields. [testing/system/TEST_LOG.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/testing/system/TEST_LOG.md) |
| [#117: Physical mission-readiness run on the final implementation](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) | OPEN | Open: incomplete and checklist stale | Physical/timing/calibration/course acceptance remains pending. Its reset, IMU-start, finish, watchdog and task-timing instructions conflict with the current contracts. [testing/mission-readiness.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/testing/mission-readiness.md) |
| [#123: S4: retarget hump-height calibration to the lower ramp (≤ 30 mm)](https://github.com/shauniee/Autonomous-Robotic-Car/issues/123) | CLOSED | Documentation completed; physical work transferred | <=30 mm/lower-ramp documentation is present. Closing explicitly transferred ramp/threshold/reference trials to 117; those are not done. [docs/architecture/subsystem4.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/docs/architecture/subsystem4.md) |
| [#124: Add videos/ evidence folder per course template](https://github.com/shauniee/Autonomous-Robotic-Car/issues/124) | CLOSED | Completed by existing owning guide | Video rules/index live in week13-plan.md; a duplicate videos/README.md is unnecessary. Actual recorded-video acceptance remains 117; index contains only a placeholder. [docs/week13-plan.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/docs/week13-plan.md) |
| [#129: docs: align the project plan with the simplified three-task architecture](https://github.com/shauniee/Autonomous-Robotic-Car/issues/129) | OPEN | Substantially implemented; docs cleanup remains | Task/core/authority/state contracts and most guides are reconciled, but stale plan/readiness/117 content prevents the no-competing-plans acceptance criterion. [docs/architecture.md](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/docs/architecture.md) |
| [#130: refactor: move essential stop checks into Motion and remove the Safety task](https://github.com/shauniee/Autonomous-Robotic-Car/issues/130) | OPEN | Software implemented; physical evidence separate | Independent GP21/stop/expiry output guard, inhibit/reset handshake and brake-order tests exist; no Safety task. Stall guard is deliberately off for current trial, and hung Motion/kernel protection is not claimed. [app_program/subsystem2/subsystem2.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/subsystem2/subsystem2.c) |
| [#131: refactor: consolidate Mission, line/barcode and obstacle coordination](https://github.com/shauniee/Autonomous-Robotic-Car/issues/131) | OPEN | Software implemented; physical evidence separate | Mission extracted; line/barcode/sonar steps, seven handlers, junction-once actions, bounded recovery/bypass and outgoing tracking are present. Bypass remains gated. [app_program/mission/mission.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/mission/mission.c) |
| [#132: refactor: consolidate bounded terrain acquisition and reporting on the I/O core](https://github.com/shauniee/Autonomous-Robotic-Car/issues/132) | OPEN | Software implemented; on-target load unverified | Core-0 I/O steps Terrain/Reporting, coherent age/impact, mapping handoff and cross-core event mutex exist; no replacement Terrain/Reporting tasks. Physical freshness/stack/WCET still need measurement. [app_program/common/io_task.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/common/io_task.c) |
| [#133: test: validate the simplified looping mission and record refactor evidence](https://github.com/shauniee/Autonomous-Robotic-Car/issues/133) | OPEN | Software integration done; full issue partial | Exactly three application tasks and repeating behavior have host/build records through INT-49; physical mixed mission and timing/stack acceptance are explicitly part of the issue and remain missing. [app_program/app_main.c](https://github.com/shauniee/Autonomous-Robotic-Car/blob/565bae7e95668d172fe4b632a14209afc4b4b548/app_program/app_main.c) |

## Plan stages

| Stage | Software on current main | Remaining acceptance / correction |
| --- | --- | --- |
| 0 / 129 | Most contract/ownership reconciliation exists | Partial: fix current contradictions, stale baseline and procedures |
| 1 / 130 | Independent Motion stop, expiry, inhibition, local reset and policy fixtures implemented | Measure button-to-output brake and physical stopping; validate fault behavior on the final image |
| 2 / 131 | Bounded Mission, seven states, junction lifecycle and stationary sweep implemented | Mounted line/junction/action/recovery trials; old moving-sweep wording is superseded |
| 3 / 132 | Core-0 I/O, bounded calibration, coherent ages, mapping handoff and event synchronization implemented | Measure sensor ages, timeout slack, contention and load on the board |
| 4 / 133 | Three-task startup, network start gate, local control and four-profile host/build evidence exist | Raised-wheel signs, physical vertical slice, WCET/jitter/ISR/stop and stack margins |
| 5 | Wheel and line controllers / calibration workflow exist | Physical counts/geometry/polarity/gains/turn/distance/lighting calibration not accepted |
| 6 | Barcode both orientations, runtime mapping, first-print capture and junction-once behavior implemented | Course prints, operating speed/polarity/offset and all four physical navigation actions |
| 7 | Bounded IMU, hump slowdown, height/count/max, quality rejection and retained results implemented | <=30 mm reference calibration, error/threshold agreement and actual single-wheel traces |
| 8 | Median groups, qualified edges, uncertainty and bounded bypass helpers implemented | Partial: bypass is disabled; physical envelope, measured forward/side coverage and original-line/rear-edge validation remain |
| 9 | Static build/message evidence and event tooling exist | Full mixed-course trials, video, live timing/memory and three consecutive unchanged-image passes missing |

## Documentation and acceptance gaps

1. **Plan baseline and implementation status lag main.** The pinned 2026-10-29
   baseline is useful history, but the current-status sections still call merged
   work local/uncommitted. PRs 139–141 contain that software on main. Preserve
   history and add a clear current status rather than treating old snapshots as
   the present implementation.
2. **Recovery wording is obsolete.** plan.md still mentions a moving +/-70-degree
   sweep as remaining work. Current specs/config/code use stationary wider
   pivots, +/-100 degrees, 180 mm/s, with bounded wait/alignment. That is an
   intentional later user decision, not a missing implementation.
3. **Terrain result reset is obsolete.** The plan still proposes clearing results
   on a new run and calls that choice pending. Later specs retain results over
   starts/stops until reboot; current code follows retention.
4. **Scan settings are set, despite plan wording.** Current software uses
   three-sample groups, minimum two valid, two adjacent rays, 30/60 mm depth
   hysteresis and a proposed 10 mm model uncertainty. The claim that production
   settings are unset is stale. Their physical calibration is still missing.
5. **Bypass is actually gated off.** `MISSION_BYPASS_RELEASED=0`; code presence and
   synthetic tests do not mean the normal robot will bypass.
   `MISSION_RECOVERY_RELEASED=1` and `MOTION_STALL_GUARD_ENABLED=0` are the current
   trial settings. `APP_PHYSICAL_RELEASE=0` is only declared/asserted and is not
   a universal output-inhibit gate; do not interpret its name as proof that all
   motion is disabled.
6. **Issue 117's procedures conflict with the active contracts.** It asks for
   GP20 reset, ordinary IMU failure blocking start, finish/MR-34 completion, old
   s3_input/s3_mission timing windows, and enabled watchdog injection. Current
   behavior is eligible GP21-release reset plus fresh GP20 start, report-only
   ordinary IMU failure after bounded startup, endless/local-stop operation and
   no enabled application reset watchdog. MR-34 is retired; MR-40 is the new
   endless-loop check. The local readiness file also retains minimum-25-mm
   crossing/moving-recovery wording that later decisions supersede. Reconcile
   these before accepting a run against the checklist.
7. **Closed 113 does not establish owner sign-off.** `robot_data.h` still says
   bit positions await owner confirmation, and issue discussion requests it.
   The layout/tests agree, but no per-owner confirmations were found in the
   retrieved issue discussion. Obtain/locate approvals; do not invent them.
8. **Closed physical-test issues are unfinished work.** 32, 47, 59, 60 and 61
   were closed during cleanup. Their calibration/raw-data/course acceptance is
   not demonstrated by the current logs. 123 explicitly transferred its
   physical remainder to 117. 38 also lacks an enabled, accepted bypass.
9. **Completion checker evidence is stale.** On current main,
   `python3 testing/check_plan_completion.py` exits **2**, reporting source
   mismatch against the last `*completion.json` (INT-25). plan.md says exit 1
   is expected while blocked; that is not today's result. Newer INT-47/49 checks
   exist but do not refresh the checker’s completion record. Append a fresh
   manifest/stage audit; never edit historical evidence to make it pass.
10. **Final submission evidence is not live acceptance.** SYS-11/12 and recent
    builds provide static sizes, not peak RAM, stack high-water marks, ISR/WCET
    or height/width errors. The video index has a placeholder, not actual
    indexed demonstrations. Refresh final source/layout accounting after later
    fields and record the required board measurements.

## Recommended next actions

1. Reconcile plan.md, issue 117 and future readiness procedures with the current
   source and accepted decisions; append a fresh completion audit/manifest.
2. Review 130–132 for software-scope closure, with explicit links transferring
   on-target acceptance to 117/133. Keep 129 open until the doc contradictions
   are resolved; keep 117/133 open until their acceptance work is recorded.
3. Recover/obtain the owner confirmations for 113 and track all unfinished
   physical work from the cleanup-closed issues in 117, rather than silently
   treating those closures as passes.
4. Calibrate/measure the final robot in checklist order. Validate bypass coverage
   and geometry before enabling its release. Record fault/trial settings with
   every flashed image and run.

## Verification and limits

- GitHub issue metadata, bodies, comments and available closure records read
  through the authenticated GitHub CLI; all 56 issues have a row above.
- Code review used a clean git archive of the exact main commit stated above.
- The audited application, scripts, build/config inputs and pure tests are
  byte-identical to the INT-49 tested integrated source `545a6a3` (checked with
  `git diff --quiet` over those paths). Its recorded host/dashboard/CLI/HTTP
  and four firmware-profile results were reused; no redundant rerun claimed.
- Completion checker executed once for this review: exit 2, stale source
  identity. No physical pass is inferred from this check.
- This is an implementation/status audit, not a fresh line-by-line defect audit
  or hardware trial. No new builds, robot commands, issue edits, commits, pushes
  or merges were performed. Closed issue discussions may refer to old designs;
  current owning guides and later user decisions take precedence.
