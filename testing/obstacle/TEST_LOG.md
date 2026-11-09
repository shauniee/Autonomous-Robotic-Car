# Obstacle test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)
- [Measurements](#measurements)

## Test cases

| ID | Test | Method | Expected |
| --- | --- | --- | --- |
| S5-U03 | Range groups: no echo, outliers, minimum-valid count, boundaries | Host | Unknown groups never become clearance |
| S5-U04 | Stationary profiles on 100/150/200 mm synthetic faces | Host | Model-consistent width; invalid rays refused |
| S5-U05 | +/-20 degree servo limit | Host | Ray bounds and clamping hold; superseded by S5-U09 |
| S5-U06 | Slow servo slew | Build, host | Settle covers the move; cancel replaces the target |
| S5-U07 | Nine scan positions, 5 degree spacing | Build, host | Each group publishes before the next target |
| S5-U08 | S5-U07 rerun, equal coarse and fine spacing allowed | Build, host | Pass |
| S5-U09 | Full sweep on 100-200 mm boxes from 110-150 mm, simulated bypass | Host, build | Bypass can start on ideal rays |
| S5-U10 | Servo and sonar bench | TEST image, motors inhibited | Horn clears at both limits; 0 degrees faces forward; positive looks left; ranges match a tape; a box gives a profile |
| S5-U11 | Reported width against box width | TEST image, GP20 scan | None set before the run |
| S5-U13 | S5-U11 rays through the estimator, before and after the edge change | Host replay | None set |
| S5-U14 | S5-U11 rays against edge thresholds | Host replay | None set |
| S5-U15 | Reported box position against box offset | TEST image, GP20 scan | None set before the run |

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-27 | Obstacle CR-111 ray/profile/side-watch host regression + 4 profile builds | `b438bcc` | CI | PASS (host, build) | [Workflow run 40518950503](https://github.com/shauniee/Autonomous-Robotic-Car/actions/runs/40518950503) | Packed fields round-trip signed angles -60..60 and full 16-bit range; simulated obstacle geometry; NOT physical servo/echo tests |
| 2026-10-27 | Obstacle CR-111 Cortex-M0+ sizeof probe | `b438bcc` | Not recorded | PASS (build) | Same workflow, Obstacle sizeof step | `ObstacleRay` 3 bytes, `ObstacleProfile` 14 bytes; side-watch holds 8 meaningful bits in one 32-bit atomic word |
| 2026-10-29 | S5-U03 / S5-U04 | `0448890`+ | Not recorded | PASS (host) | — | PASS (host only). Host output; Bounded median/edge/model helpers and invalid/no-echo cases; synthetic fixture settings, not enabled production calibration |
| 2026-10-30 | S5-U03/U04 | `62ee522`+ | Codex | PASS (host, build) | At tag | INT-25: unused `s5_profile_build` and `s5_fine_angles` removed; helpers re-checked |
| 2026-10-30 | S5-U05 | `cc3edb3`+ | Not recorded | PASS (host) | — | PASS (host only). Host suite output; no physical servo/echo or bypass acceptance |
| 2026-10-30 | S5-U06 | `8be851b`+ | Codex | PASS (host, build) | — | PASS (build / host regressions only). Build, host output; image checksum in manifest, not flashed; slew timing and Echo not physically tested |
| 2026-10-30 | S5-U07 | `8be851b`+ | Codex | FAIL (host, build) | — | FAIL (build / host compile). Strict coarse/fine spacing assertion; output preserved above |
| 2026-10-30 | S5-U08 | `8be851b`+ | Codex | PASS (host, build) | — | PASS (build / existing host regressions only). Build, host output; image checksum in manifest, not flashed; no physical scan/Echo acceptance |
| 2026-11-02 | S5-U09 | `79c13f1`+ | Claude | PASS (host, build) | At tag | Ideal rays gave at most 136 mm clearance against 300 mm, so limit +/-50 and clearance from each free ray; not physical |
| 2026-11-02 | S5-U10 | Not run | — | PENDING | None | Motors inhibited with `./flash.sh test`; covers MR-17 and MR-18 |
| 2026-11-03 | S5-U11 | `e576760`+ | Nicholas, Claude | OBSERVED (car) | [+/-50](logs/2026-11-03_S5-U11-serial-sweep50.txt.gz), [+/-60](logs/2026-11-03_S5-U11-serial-sweep60.txt.gz), [course stop](logs/2026-11-03_S5-U11-course-stop-scan-mqtt.txt.gz) | MEASURED, no target set. 19 of 23 scans gave a width, see Measurements. MR-17, MR-18, MR-30, MR-31 stay open |
| 2026-11-03 | S5-U13 | `e576760`+ | Claude | OBSERVED (host) | Same 23 scans as S5-U11 | MEASURED (host replay). Width from 19 of 23 scans before the edge change, 22 of 23 after; the 19 widths are unchanged |
| 2026-11-03 | S5-U14 | `e576760`+ | Claude | OBSERVED (host) | Same 23 scans as S5-U11 | MEASURED (host replay), no pass or fail. Edge thresholds, see Measurements. Servo settle, empty-scene reference and two-distance scans not tested |
| 2026-11-05 | S5-U15 | `56f7656`+ | Nicholas, Claude | OBSERVED (car) | [serial log](logs/2026-11-05_S5-U15-serial.txt.gz) | 180 mm box at 160 mm, centre/right/left. 13 scans: +25 deg hit in 13, +30 in none, so left edge always 27.5 deg; right -25 to -42.5. Rule would choose left in 12 of 13 |
| 2026-11-05 | S5-U16 | `3fd95ba`+ | Nicholas | OBSERVED (car) | [scan.log](logs/2026-11-05_S5-U16/scan.log) | Stationary scans, motors inhibited. Echoes at twice the edge range (389 mm vs 196/206 mm) appear only with the box: ghost echo. Clear scan: left 31, right 40 mm |

## Measurements

S5-U11. Sizes and distances are the user's, by tape, and approximate. Rays at the box edge flip between scans, and the largest errors read too wide.

| Box | Distance | Sweep | Scans | Result | Width (mm) | Error (mm) | Reported uncertainty (mm) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 220 mm | 180 mm | +/-50 | 7 | 5 | 228, 228, 216, 227, 272 | +8, +8, -4, +7, +52 | 54-66 |
| 150 mm | Not confirmed; 0 degrees read 108-119 mm | +/-50 | 7 | 7 | 150, 152, 150, 139, 140, 138, 139 | 0, +2, 0, -11, -10, -12, -11 | 45-46 |
| 150 mm | About 115 mm | +/-50 | 2 | 1 | 198 | +48 | 50 |
| 150 mm | Tape not confirmed; 0 degrees read 115-116 mm | +/-60 | 7 | 6 | 163, 162, 147, 179, 194, 162 | +13, +12, -3, +29, +44, +12 | 55-58 |

S5-U14. The 23 S5-U11 scans, against the box widths above.

| Enter / exit (mm), adjacent rays | Scans with width | Mean abs error, one sweep | Lowest / highest error, one sweep | Mean abs error, narrower of two |
| --- | --- | --- | --- | --- |
| 30 / 60, 2 | 22 of 23 | 18.2 | -12 / +52 | 10.9 |
| 30 / 40, 2 (current) | 23 of 23 | 18.0 | -14 / +52 | 11.2 |
| 30 / 30, 2 | 23 of 23 | 19.7 | -51 / +52 | 15.0 |
| 30 / 40, 3 | 23 of 23 | 18.0 | -14 / +52 | 11.2 |
