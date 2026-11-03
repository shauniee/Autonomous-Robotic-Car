# Terrain test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

Owner: Darrius / terrain. Template S4-U01 and S4-U02 are retired, not reused. One row per run. Write-ups from before 2026-11-03, and build and host output, are at git tag `test-logs-full-2026-11-03`: `git show test-logs-full-2026-11-03:testing/terrain/TEST_LOG.md`. `+` after a commit means it also had uncommitted changes.

## Test cases

| ID | Test | Method | Expected |
|---|---|---|---|
| S4-U03 | Wireless TEST compile after the boot budget change | Build | Linked image |
| S4-U04 | Retry S4-U03 after generated objects are invalidated | Build | Linked ELF and UF2 |
| S4-U05 | 20 degree climb, then a -60 degree descent skipping the crest band | Host | DESCENDING on the second descent sample; one valid hump |

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-27 | Terrain CR-110 host float-reference replay + 4 profile builds | `e8f5a8d` | GitHub Actions | PASS | [Workflow run 40518795052](https://github.com/shauniee/Autonomous-Robotic-Car/actions/runs/40518795052) | Synthetic flat/hump/impact/calibration scenarios compared with original float reference; NOT real IMU measurements |
| 2026-10-27 | Terrain CR-110 Cortex-M0+ sizeof probe | `e8f5a8d` | arm-none-eabi-gcc | PASS | Same workflow, Terrain sizeof step | `TerrainContext` 32 bytes, `TerrainCalibSum` 9 bytes (3 x 23-bit signed), `TerrainFilterQ16` 12 bytes |
| 2026-10-30 | S4-U03 | `8be851b`+ | Codex | FAIL (build) | At tag | Linker could not find generated objects after a profile change |
| 2026-10-30 | S4-U04 | `8be851b`+, [manifest](logs/2026-10-30_S4-U04-manifest.json) | Codex | PASS (build, host) | At tag | S4-U03 retry; boot calibration budget 2 to 5 s; not flashed |
| 2026-11-02 | S4-U05 | `4b1cc19`+ | Codex | PASS (host) | At tag | Failed before the fix; synthetic ramp, not measured geometry |
