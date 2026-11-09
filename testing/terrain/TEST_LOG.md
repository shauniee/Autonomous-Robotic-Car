# Terrain test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

## Test cases

| ID | Test | Method | Expected |
| --- | --- | --- | --- |
| S4-U03 | Wireless TEST compile after the boot budget change | Build | Linked image |
| S4-U04 | Retry S4-U03 after generated objects are invalidated | Build | Linked ELF and UF2 |
| S4-U05 | 20 degree climb, then a -60 degree descent skipping the crest band | Host | DESCENDING on the second descent sample; one valid hump |
| S4-U06 | Raw USB capture parser, loss and timeout checks; terrain regression | Host | Four Python tests and terrain pass |
| S4-U07 | Roll-based terrain regression with the IMU capture and field-tool checks | Host | Terrain regression and capture checks pass, including signed roll |
| S4-U08 | Full host suite after S4-U07 on Windows | Host | Whole suite passes |
| S4-M01 | Raw USB samples over a measured hump | Physical | X/Y/Z, pitch/roll and encounter state captured without loss; pending |

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-27 | Terrain CR-110 host float-reference replay + 4 profile builds | `e8f5a8d` | CI | PASS (host, build) | [Workflow run 40518795052](https://github.com/shauniee/Autonomous-Robotic-Car/actions/runs/40518795052) | Synthetic flat/hump/impact/calibration scenarios compared with original float reference; NOT real IMU measurements |
| 2026-10-27 | Terrain CR-110 Cortex-M0+ sizeof probe | `e8f5a8d` | Not recorded | PASS (build) | Same workflow, Terrain sizeof step | `TerrainContext` 32 bytes, `TerrainCalibSum` 9 bytes (3 x 23-bit signed), `TerrainFilterQ16` 12 bytes |
| 2026-10-30 | S4-U03 | `8be851b`+ | Codex | FAIL (build) | At tag | Linker could not find generated objects after a profile change |
| 2026-10-30 | S4-U04 | `8be851b`+ | Codex | PASS (host, build) | At tag | PASS (build, host). S4-U03 retry; boot calibration budget 2 to 5 s; not flashed |
| 2026-10-31 | S4-U09 | `5e63fe0` | Codex | PASS (host, build) | — | Terrain header build compatibility |
| 2026-11-02 | S4-U05 | `4b1cc19`+ | Codex | PASS (host) | At tag | Failed before the fix; synthetic ramp, not measured geometry |
| 2026-11-02 | S4-U10 | `d97e3d9`+ | Nicholas, Claude | FAIL (car) | [2026-11-02_S4-U10/](logs/2026-11-02_S4-U10/) | Humps counted on the wrong encounters. Hump count wrong |
| 2026-11-02 | S4-U11 | `d97e3d9`+ | Nicholas, Claude | FAIL (car) | [2026-11-02_S4-U11/](logs/2026-11-02_S4-U11/) | Hump baseline moved by pivots. Tracking off course, not analysed; `humps=5` against 1 crossed |
| 2026-11-03 | S4-U12 | `7275347` | Claude | OBSERVED (static) | — | IMU calibration failed because the car moved during it (INT-154 lines 22501-26351, counts 5 and 157 backward); firmware restarts sampling, times out at 5 s |
| 2026-11-03 | S4-U13 | `7275347` | Claude | OBSERVED (host) | — | Hump state held up to 9.9 s on older images (INT-156, 157). Current code ends an encounter at `HUMP_MAX_TRAVEL_MM` (host cases). Needs a car run |
| 2026-11-04 | S4-U06 | `45f5978` | Codex | PASS (host) | Original branch commit | Capture-tool and terrain tests; firmware and hardware unverified |
| 2026-11-04 | S4-U07 | `7275347`+ | Codex | PASS (host) | No host output retained | Current roll-based terrain regression; four capture checks including signed roll; five field-tool checks; no firmware/hardware claim |
| 2026-11-04 | S4-U08 | `7275347`+ | Codex | FAIL (host) | No host output retained | C regressions and MQTT checks passed; suite stopped at `test_check_barr.py:114` path assertion on Windows |
