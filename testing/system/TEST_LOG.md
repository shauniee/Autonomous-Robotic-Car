# System and physical test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

## Test cases

| ID | Requirement | Steps | Expected / measurements |
| --- | --- | --- | --- |
| SYS-03 | Inhibited USB-only checks | USB sole source; flash TEST image by BOOTSEL; capture startup/self-tests/BENCH console; exercise GP20 black/white/start and GP21 | No motor movement; actual test lines, ADC readings, capture stage/cues logged |
| SYS-04 | Battery calibration and polarity | Flash normal; unplug USB, connect power bank only; wheels raised; capture black then white; start on visible line; GP21 stop | Correct chirps, contrast, forward polarity; reset requires separate start; measured motor-output stop <=50 ms target |
| SYS-05 | Straight/curve, loss and reboot | Normal firmware, USB disconnected; low-speed runs, controlled loss; fresh reboot | Repeated tracking; bounded <=100 ms commanded loss policy; physical stop separately measured; reboot inhibits and clears refs |
| SYS-06 | Task-stall watchdog | Controlled bench fault injection with wheels raised, sole power source | No feed without both progressing; measure reset time and inhibited reboot; requires suitable operator instrumentation |
| SYS-07 | Proportional baseline and recovery | Battery sole source; record geometry/calibration and flashed image; repeat straight/curves and left/right controlled loss; test unknown direction and persistent broad black | Measured P tracking; two visible acquisitions before bounded resume; requested turn <=100 ms; motor-output/physical stop measured separately; thresholds/gains not prevalidated |
| SYS-08 | Future barcode | Planned after framing/mapping and decoder implementation; known samples, partial/reverse/speed/duplicate/overflow cases | One valid mapped event per encounter; no guessed turns; navigation completion requires encoders |
| SYS-09 | Actual Pico wireless TEST connectivity | Mac broker/subscriber on same private 2.4 GHz network; USB alone, carrier switch on; flash wireless TEST; exercise black/white/start/stop | Approximately 500 ms JSON, current ADCs/references/age, null before calibration, correct states and test_inhibited=true; no motor output; record boot console, revision, UF2 checksum and MQTT log |
| SYS-10 | Bank-only wireless following and network loss | Flash wireless normal; unplug USB then bank only; recalibrate; track/stop/reset; interrupt/restart broker and WiFi while observing safely | Current telemetry and latest snapshot on reconnect, no backlog; healthy local autonomy/GP21 preserved across loss; physical stop/recovery timing measured separately; no USB attached |
| SYS-11 | CR minimum-bit final RAM and message size (#114) | Section-4 build and `arm-none-eabi-size`; target-compiler `sizeof` probes for every subsystem cross-checked with `arm-none-eabi-nm -S` on the linked objects; `test_mqtt_frame` | text/data/bss with commit and toolchain; every persistent instance equals probe size x count; frame lengths equal [architecture](../../docs/architecture.md#mqtt-binary-protocol-schema-version-2) |
| SYS-12 | Current resource audit | INT-24 host, four builds and target probes | Frame lengths stay 23/11/5/3/2 bytes; persistent objects match the probes |

## Results

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-08 | SYS-03/04/05/06 | To record before trial | Not recorded | PENDING | — | No connected Pico; physical calibration/tracking/stop/reset cannot be performed in this session |
| 2026-10-08 | SYS-07/08 | To record before trial | Not recorded | PENDING | — | Proportional refactor software checks only; barcode deferred, no physical trial performed |
| 2026-10-08 | SYS-09/10 | To record before trial | Not recorded | PENDING (host, build) | — | MQTT profiles built and host/local broker checks passed; no Pico flash, radio connection or physical trial performed for this addition |
| 2026-10-27 | SYS-11 | `caa7f8c` | Not recorded | OBSERVED (build) | — | MEASURED (build). Offline normal: text 72272, data 552, bss 23716; sizes match linked objects; frames 23/11/5/3/2 bytes. Before the restructure |
| 2026-10-30 | SYS-12 | `62ee522`+ | Codex | PASS (host, build) | — | PASS (host, build, static). Wireless normal: text 376608, data 392, bss 84120. Stacks, peak RAM, T15 not measured |
| 2026-10-30 | SYS-12 | `62ee522`+ | Codex | PASS (host, build) | — | PASS (host, build, static). INT-25 refresh. Wireless normal: text 376648, data 392, bss 84128 |
| 2026-10-30 | MR-17 | `8be851b` | Not recorded | PENDING (car) | — | User reports readings matched at 100/250/500/1000 mm and no echo in open space; values and image checksum not recorded. Repeat before PASS |
