# S2 Motion test log

Owner Mahesha / 2503973. Template example S2-U01/U02 are retired, not reused. Retired definitions: S2-U01 example values in 0..100; S2-U02 pool/slot bounds. No template result was recorded.

## Contents

- [Test cases](#test-cases)
- [Results](#results)

## Test cases

| ID | Test / requirement | Method / steps | Expected |
|---|---|---|---|
| S2-U03 | Freshness, stop gate; IF-01/02 | On-target TEST=1 self-check | `[TEST] S2-U03 PASS`; outputs remain inhibited |
| S2-U04 | Duty bounds, expiry, uint32 wrap; [Motion inhibition contract](../../docs/architecture/MOTION.md#business-logic) | Historical host policy check; runner retired | Invalid/expired/stopped request rejected |
| S2-U05 | Real Motion service / kernel contract | Historical real-service host check, normal and TEST policies; runner retired | Boot latch, reset invalidation, expiry latch, independent stop on mutex failure; TEST always brakes |

## Results

| Date | ID | Source | Tester | Result | Evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-08 | S2-U04 | Manifest in integration/logs | Codex host run | PASS | ../integration/logs/2026-10-08_host-checks.txt | Logic only |
| 2026-10-08 | S2-U05 | Manifest in integration/logs | Codex host run | PASS | ../integration/logs/2026-10-08_host-checks.txt | Real service with fake kernel/output, both profiles |
| 2026-10-08 | S2-U03 | Not yet run on Pico | Pending operator | PENDING | none | Built only; no on-target pass |

## 2026-10-27 — Minimum-bit MotionCommand candidate (#109)

| Date | ID | Source | Tester | Result | Evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-27 | S2 CR-109 host regression + 4 profile builds | `acb1e416` | GitHub Actions | PASS | [Workflow run 40518529209](https://github.com/shauniee/Autonomous-Robotic-Car/actions/runs/40518529209) | Includes test_motion_control, test_mission, full pure test suite; NOT on-target motor operation |
| 2026-10-27 | S2 CR-109 Cortex-M0+ sizeof probe | `acb1e416` | arm-none-eabi-gcc (Docker build image) | PASS | Same workflow, Motion sizeof step | `MotionCommand` 12 bytes (4 sampled + 2 target + 2 speed/action + 2 steering/mode + 2 padding); `MotionHealth` 12 bytes (4 + 4 + 1 flag + 3 padding) |

The 2000 mm DISTANCE bound and signed Q7 steering require S2/D07 owner review and physical motor calibration before merging. This software evidence does not claim stable wheel timing, stop latency, or tuning on the robot.
