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
| S2-U06 | Reset predicate for `motion_stop_reset_allowed` | `testing/host/run.sh` | Refuses reset until every observed stop request has a brake acknowledgement |

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-08 | S2-U03 | Not yet run on Pico | — | PENDING (build) | none | Built only; no on-target pass |
| 2026-10-08 | S2-U04 | Manifest in integration/logs | Codex | PASS (host) | at tag | Logic only |
| 2026-10-08 | S2-U05 | Manifest in integration/logs | Codex | PASS (host) | at tag | Real service with fake kernel/output, both profiles |
| 2026-10-27 | Motion CR-109 host regression + 4 profile builds | `acb1e41` | CI | PASS (host, build) | [Workflow run 40518529209](https://github.com/shauniee/Autonomous-Robotic-Car/actions/runs/40518529209) | Includes test_motion_control, test_mission, full pure test suite; NOT on-target motor operation |
| 2026-10-27 | Motion CR-109 Cortex-M0+ sizeof probe | `acb1e41` | Not recorded | PASS (build) | Same workflow, Motion sizeof step | `MotionCommand` 12 bytes (4 sampled + 2 target + 2 speed/action + 2 steering/mode + 2 padding); `MotionHealth` 12 bytes (4 + 4 + 1 flag + 3 padding) |
| 2026-10-29 | S2-U06 | `407cbfa` | Codex | PASS (static) | — | PASS (policy only). Host run |
| 2026-10-31 | S2-U07 | `8981d04` | Nicholas, Codex | OBSERVED (car) | [2026-10-31_S2-U07/](logs/2026-10-31_S2-U07/) | User-reported encoder revolution count. User-reported: 2513 counts for one wheel revolution, 6.981 times the configured 360 |
| 2026-10-31 | S2-U08 | In manifest | Nicholas, Codex | OBSERVED (car) | [2026-10-31_S2-U08/](logs/2026-10-31_S2-U08/) | MEASURED. Operator-assisted encoder calibration. Manual 500 mm roll: left 11.666, right 11.632, mean 11.649 counts/mm; distance not independently measured |
| 2026-10-31 | S2-U09 | `8981d04`+ | Codex | PASS (host, build) | [2026-10-31_S2-U09/](logs/2026-10-31_S2-U09/) | Measured rolling-scale application. Applies S2-U08 |
| 2026-11-01 | S2-U10 | `b0c3d1b`+ | Nicholas, Claude | OBSERVED (car) | [2026-11-01_S2-U10/](logs/2026-11-01_S2-U10/) | MEASURED. Wheel speed at a 200 mm/s request. Raised wheels 200 mm/s at 22-24% duty; floor 203-206 mm/s at 28-30% |
| 2026-11-01 | S2-U11 | image `5e2a7ed` | Nicholas, Claude | OBSERVED (car) | [2026-11-01_S2-U11/](logs/2026-11-01_S2-U11/) | MEASURED; PASS (host). Wheel feedforward fit. Raised wheels at 60, 100, 140 mm/s; feedforward fitted |
| 2026-11-01 | S2-U12 | image `bedea7c` | Nicholas, Claude | OBSERVED (car) | [2026-11-01_S2-U12/](logs/2026-11-01_S2-U12/) | MEASURED. 100 mm/s step with fitted feedforward. Raised 100 mm/s step, peak 112 mm/s; wheels alternate below 60 mm/s |
| 2026-11-01 | S2-U13 | image `16c6ae0` | Nicholas, Claude | OBSERVED (car) | [2026-11-01_S2-U13/](logs/2026-11-01_S2-U13/) | MEASURED; PASS (host). Lower wheel KP. KP 0.0001 removes the alternation; raised wheels only |
| 2026-11-03 | S2-U14 | `7275347` | Claude | OBSERVED (static) | — | Count spikes follow capture gaps, not encoder faults (S4-U10 line 11667, INT-154 line 28918). WHEEL peak 210 mm/s or less. HUMP `encoder_ok=0` counts unexplained |
| 2026-11-03 | S2-U15 | `ebb3a2c` | Claude | PASS (host, build) | — | Stall guard on for `FULL_COURSE=1`. Replay found one false stop at a 3 mm/s target (INT-156 line 1614), so targets under 10 mm/s no longer count; floor provisional |
| 2026-11-04 | S2-U16 | `d87c6f9` | Claude | PASS (host, build) | — | Jerk-limited planned moves; GP21 still immediate. Host passes at 100, 80, 60 mm/s (test plant). Full course `4c67d85e`: text 386824, bss 83808 |
