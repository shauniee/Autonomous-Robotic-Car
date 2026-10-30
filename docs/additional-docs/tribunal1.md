# Tribunal 1 summary (INF2004 Week 8, group CSC01)

## Contents

- [Purpose](#purpose)
- [Ownership](#ownership)
- [What the team defended (Week 6 design, unchanged)](#what-the-team-defended-week-6-design-unchanged)
- [What changed: minimum-bit requirement](#what-changed-minimum-bit-requirement)
- [Issues and decisions](#issues-and-decisions)
- [Risk register (Version B)](#risk-register-version-b)
- [Assumption closure](#assumption-closure)
- [Test plan (Version B, all Open)](#test-plan-version-b-all-open)
- [Remaining gaps](#remaining-gaps)

Historical record only: task splits, leases, queue designs and proposed numerical targets below are not competing current contracts. The [central plan](../../plan.md) and [architecture](../architecture.md) supersede them for implementation.

Condensed record of the Week 8 Tribunal 1 submission. Current requirements and open decisions live in [project-specs](../project-specs.md); owners and pins in [team](../team.md); coordination rules in [architecture](../architecture.md).

## Purpose

Defend the Week 6 architecture, justify changes, and show readiness for partial integration (Week 10 demo). The submission had three stages: pre-session AI ideation per buddy (Version A), a live Zoom-updated version (Version B), and a post-session AI gap check of A and B (Version C).

## Ownership

| Buddy | Subsystem | Guide |
| --- | --- | --- |
| Shaun | S1 WiFi, MQTT, telemetry | [subsystem1](../architecture/subsystem1.md) |
| Mahesha | S2 motors, encoders, PID | [subsystem2](../architecture/subsystem2.md) |
| Nicholas | S3 IR line following, barcode; Mission/Safety | [subsystem3](../architecture/subsystem3.md) |
| Darrius | S4 IMU, terrain, hump height | [subsystem4](../architecture/subsystem4.md) |
| Jia Jing | S5 ultrasonic, servo, obstacle profiling | [subsystem5](../architecture/subsystem5.md) |

## What the team defended (Week 6 design, unchanged)

- Motion is the only motor writer; commands expire; Safety keeps stop authority; Mission owns navigation, bypass and acceptance of configuration changes.
- Networking (S1) has no movement authority; control does not depend on WiFi.
- Line snapshots are separate from queued barcode events; front obstacle ranges are separate from side-scan profiles; hump detection is separate from height validity.
- Task priorities and RTOS interfaces were retained (no change in task ownership, no hardware amendment).
- Evidence cited at the time: configured periods (S3 input 10 ms; IR ADC sampling, Mission and Motion 20 ms), command/feedback/stop code, inhibited Mission and Motion checks, synthetic line/barcode and obstacle checks, terrain logic checks without the IMU, and inhibited MQTT mapping/reload observations. End-to-end response and the 50 ms brake-output target were unmeasured; no physical pass was recorded.

## What changed: minimum-bit requirement

The session introduced a minimum-bit data requirement. Decisions recorded in Version B:

- JSON replaced by a binary format (Shaun).
- Every field audited per subsystem and restructured to binary types instead of ordinary enums, booleans and floats (all owners).
- Fault enums restructured into bit values so simultaneous faults stay independently representable.
- Float and signed-integer values (normalised today) audited so values are not truncated or lost (Nicholas, Mahesha, Jia Jing, Darrius).
- Terrain compromise (Darrius): live values in 16-bit fixed point, final result packed into 7 bits.
- Obstacle scan states as 3-bit values and quantised profiles (Jia Jing).
- No change: Mission request priority and the subsystem priority hierarchy; subsystems still communicate through RTOS interfaces.
- Memory and core usage: report RAM, message sizes and packing/unpacking CPU overhead for the final implementation.

**Requirement update (2026-10-27):** a comparison of the revised implementation with the original is no longer required. Teams implement the additional requirements and provide evidence for the resulting final implementation only.

## Issues and decisions

| # | Issue | Decision / outcome | Follow-up owner |
| --- | --- | --- | --- |
| 1 | Minimum-bit representation is not yet a complete data contract | Keep the objective as the smallest justified representation. Document bounds, units, signedness, scale, rounding, invalid codes and overflow per field; approve widths only after boundary and precision checks. Timestamps, signed speed, counters and simultaneous faults need separate treatment | Each owner |
| 2 | Packed updates can corrupt unrelated fields or publish inconsistent snapshots | Mutex-protected snapshots and concurrency tests. Use one writer or an appropriately synchronised update; protect the complete update; measure lock duration, blocking and cross-core handoffs. Risks R1 and R4 stay open (High) | Shaun, all providers |
| 3 | Encoder distance reset at every barcode | Reset proposed to avoid overflow with unknown total mission distance. Not accepted as overflow-safe: a missed barcode leaves an unbounded interval, and a shared reset can break other consumers (motion targets, barcode width, terrain, obstacle/bypass). Needs an interface review: per-consumer baselines or segment origin, reset/epoch notification, rollover handling, tests for missed/duplicate barcodes and resets during actions | Mahesha, with Nicholas, Darrius, Jia Jing |
| 4 | Representation changes may break command freshness and stopping | Keep original sensor timestamps when renewing commands; keep command acceptance, completion and measured stationary feedback distinct; a zero target does not prove stopped wheels. Verify expiry/rollover, stale commands, stop/reset races, held buttons, inhibited reboot; timestamp sample, decision, command and PWM under peak load, then measure stopping time and distance | Nicholas, Mahesha, all owners |
| 5 | JSON-to-binary needs a compatible protocol | Binary format chosen at 47:47. Define wire schema (field order, lengths, bit order, scaling, validity, version) and update firmware and laptop endpoints; test malformed/truncated input, busy/retry, reconnect and full acknowledgement slots. Keep stopped-only replacement; applied acknowledgement means the whole mapping was installed. Earlier 26-entry/four-slot MQTT observations used JSON and do not validate the binary path | Shaun, all owners |
| 6 | Physical line/obstacle behaviour is still a gate | Compressed data does not resolve IR contrast, barcode framing, junction ambiguity, no-echo handling, servo settling or clearance. Test issued prints, directions, speeds, queue occupancy/overflow, scan cancellation and centre return; verify Echo level, pin/jumper wiring and loaded supply before powered trials (R5, Echo above 3.3 V, open). Unknown/no echo stays distinct from clear space; recovery needs agreed corridor checks, limits and a safe outcome | Nicholas, Jia Jing, Mahesha |
| 7 | Terrain precision and height validity unproven | Update the maximum height only after a valid completed encounter. 16-bit/7-bit widths provisional: state units, range, error budget, rounding and invalid-height representation; check accumulation and overflow; calibrate the mounted sensor; compare repeated steady-speed runs against reference humps. The "few millimetres" criterion has no confirmed tolerance or measured result | Darrius |
| 8 | Readiness and resource architecture | Conditional partial-integration position: documented controlled integration, but changed representations are unvalidated. Record RAM/message sizes, packing cost and stack use of the final implementation, and keep R3 (stack overflow) separate from pointer/bounds errors. Each change needs a specification, positive pass criterion and linked result; Zoom timestamps record decisions only. Updated position: partially ready and integrated, regression testing across several subsystems not done, RAM/size and correctness/performance evidence planned before the Week 13 submission | Shaun, all owners |

## Risk register (Version B)

| ID | Risk | Rating | Mitigation | Owner | Status |
| --- | --- | --- | --- | --- | --- |
| R1 | Corrupted telemetry from concurrent packed updates | High/High | Mutex snapshot, concurrency tests | Shaun | Open |
| R2 | Range/precision loss | Medium/Medium | Approve bounds and precision first; specify invalid/out-of-range behaviour | Nicholas, Mahesha, Darrius, Jia Jing | Open |
| R3 | Stack overflow stops the robot mid-journey | High/High | Switch to bit system incrementally | All | Open |
| R4 | Interface mismatch from unregulated read-modify-write | High/High | Single writer or atomic/short critical section | All | Open |
| R5 | Echo voltage above 3.3 V | Medium/High | Verify before connection | Jia Jing | Open |

## Assumption closure

Assumptions A1-A13 (WiFi/MQTT recovery, encoder reliability, IR contrast, hump height, sonar profiles, electrical limits, pin wiring, barcode rules, recovery, finish rules, timing, stop/reset, geometry scaling) were marked Needs validation, Partly confirmed or Open. Only synthetic, logic or inhibited checks existed; none had a physical pass.

## Test plan (Version B, all Open)

Binary telemetry and concurrent snapshot access (Shaun); motor movement, stopping and direction with serial-log evidence (Mahesha); stationary scan gating on valid motion feedback (Jia Jing); hump rise estimate within a few millimetres of a test hump (Darrius); combined safety/timing, braking, stationary confirmation, no-echo and stop cancellation with synchronised logs (Nicholas, Mahesha, Jia Jing).

## Remaining gaps

- No completed field specification (ranges, scales, invalid encodings) for the packed layout.
- Binary wire format and laptop compatibility undefined; powered-operation reconnect untested.
- Barcode-triggered encoder reset not agreed; barcode spacing and course/finish rules unconfirmed.
- No loaded timing, physical stopping, braking or end-to-end latency measurements.
- No RAM, message-size or CPU evidence for the final implementation; no full regression.
- Mounted IMU calibration and hump accuracy; real barcode capture; sonar range and clearance geometry.
- Electrical checks (Echo level, jumper/pin continuity, loaded rails). The Green hardware-wiring status in Version B conflicts with these pending checks.
- Version C was a document-only review: it found these gaps but did not recheck code, recording or robot.
