# Motion test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

## Test cases

| ID | Test / requirement | Method / steps | Expected |
| --- | --- | --- | --- |
| S2-U03 | Freshness, stop gate; IF-01/02 | On-target TEST=1 self-check | `[TEST] S2-U03 PASS`; outputs remain inhibited |
| S2-U04 | Duty bounds, expiry, uint32 wrap; [Motion inhibition contract](../../docs/architecture.md#motion-command-contract) | Historical host policy check; runner retired | Invalid/expired/stopped request rejected |
| S2-U05 | Real Motion service / kernel contract | Historical real-service host check, normal and TEST policies; runner retired | Boot latch, reset invalidation, expiry latch, independent stop on mutex failure; TEST always brakes |
| S2-U06 | Reset predicate for `motion_stop_reset_allowed` | `testing/pure/run.sh` | Refuses reset until every observed stop request has a brake acknowledgement |

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-08 | S2-U03 | Not yet run on Pico | — | PENDING (build) | none | Built only; no on-target pass |
| 2026-10-08 | S2-U04 | Manifest in integration/logs | Codex | PASS (host) | at tag | Logic only |
| 2026-10-08 | S2-U05 | Manifest in integration/logs | Codex | PASS (host) | at tag | Real service with fake kernel/output, both profiles |
| 2026-10-27 | Motion CR-109 host regression + 4 profile builds | `acb1e41` | CI | PASS (host, build) | [Workflow run 40518529209](https://github.com/shauniee/Autonomous-Robotic-Car/actions/runs/40518529209) | Includes test_motion_control, test_mission, full pure test suite; NOT on-target motor operation |
| 2026-10-27 | Motion CR-109 Cortex-M0+ sizeof probe | `acb1e41` | Not recorded | PASS (build) | Same workflow, Motion sizeof step | `MotionCommand` 12 bytes (4 sampled + 2 target + 2 speed/action + 2 steering/mode + 2 padding); `MotionHealth` 12 bytes (4 + 4 + 1 flag + 3 padding) |
| 2026-10-29 | S2-U06 | `407cbfa` | Codex | PASS (static) | — | PASS (policy only). Host run |
