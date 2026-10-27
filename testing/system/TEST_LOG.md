# System and physical test log

Retired template definitions: SYS-01 illustrative ten-second WiFi reading; SYS-02 unfilled manual placeholder. No template result was recorded. Full mission report trials T01-T16 remain pending. No source commit or hardware result is fabricated.

## Contents

- [1. Test cases](#1-test-cases)
- [2. Results](#2-results)
- [3. Final memory and message size (SYS-11)](#3-final-memory-and-message-size-sys-11)

## 1. Test cases

| ID | Requirement | Steps | Expected / measurements |
|---|---|---|---|
| SYS-03 | Inhibited USB-only checks | USB sole source; flash TEST image by BOOTSEL; capture startup/self-tests/BENCH console; exercise GP20 black/white/start and GP21 | No motor movement; actual test lines, ADC readings, capture stage/cues logged |
| SYS-04 | Battery calibration and polarity | Flash normal; unplug USB, connect power bank only; wheels raised; capture black then white; start on visible line; GP21 stop | Correct chirps, contrast, forward polarity; reset requires separate start; measured motor-output stop <=50 ms target |
| SYS-05 | Straight/curve, loss and reboot | Normal firmware, USB disconnected; low-speed runs, controlled loss; fresh reboot | Repeated tracking; bounded <=100 ms commanded loss policy; physical stop separately measured; reboot inhibits and clears refs |
| SYS-06 | Task-stall watchdog | Controlled bench fault injection with wheels raised, sole power source | No feed without both progressing; measure reset time and inhibited reboot; requires suitable operator instrumentation |
| SYS-07 | Proportional baseline and recovery | Battery sole source; record geometry/calibration and flashed image; repeat straight/curves and left/right controlled loss; test unknown direction and persistent broad black | Measured P tracking; two visible acquisitions before bounded resume; requested turn <=100 ms; motor-output/physical stop measured separately; thresholds/gains not prevalidated |
| SYS-08 | Future barcode | Planned after framing/mapping and decoder implementation; known samples, partial/reverse/speed/duplicate/overflow cases | One valid mapped event per encounter; no guessed turns; navigation completion requires encoders |
| SYS-09 | Actual Pico wireless TEST connectivity | Mac broker/subscriber on same private 2.4 GHz network; USB alone, carrier switch on; flash wireless TEST; exercise black/white/start/stop | Approximately 500 ms JSON, current ADCs/references/age, null before calibration, correct states and test_inhibited=true; no motor output; record boot console, revision, UF2 checksum and MQTT log |
| SYS-10 | Bank-only wireless following and network loss | Flash wireless normal; unplug USB then bank only; recalibrate; track/stop/reset; interrupt/restart broker and WiFi while observing safely | Current telemetry and latest snapshot on reconnect, no backlog; healthy local autonomy/GP21 preserved across loss; physical stop/recovery timing measured separately; no USB attached |
| SYS-11 | CR minimum-bit final RAM and message size (#114) | Section-4 build and `arm-none-eabi-size`; target-compiler `sizeof` probes for every subsystem cross-checked with `arm-none-eabi-nm -S` on the linked objects; `test_mqtt_frame` | text/data/bss with commit and toolchain; every persistent instance equals probe size x count; frame lengths equal [architecture](../../docs/architecture.md#mqtt-binary-protocol-schema-version-2) |

## 2. Results

| Date | ID | Revision | Result | Reason |
|---|---|---|---|---|
| 2026-10-08 | SYS-03/04/05/06 | To record before trial | PENDING | No connected Pico; physical calibration/tracking/stop/reset cannot be performed in this session |
| 2026-10-08 | SYS-07/08 | To record before trial | PENDING | Proportional refactor software checks only; barcode deferred, no physical trial performed |
| 2026-10-08 | SYS-09/10 | To record before trial | PENDING | MQTT profiles built and host/local broker checks passed; no Pico flash, radio connection or physical trial performed for this addition |

Do not connect USB while the power bank is connected. Use camera/scope/operator records during battery trials; USB logs are collected only in the separate USB-only phase. Record actual wiring, calibrated references and exact flashed firmware checksum/revision in every result.

## 3. Final memory and message size (SYS-11)

2026-10-27. Measured at commit `caa7f8c` (head of PR #125, #113 fault bits pending owner confirmation). Its firmware sources equal `main` `1f1b13b` except one comment in `robot_data.h`. Compile and link only; no firmware was executed or flashed. Toolchain arm-none-eabi-gcc 15.3.1 (Arm GNU Toolchain 15.3.Rel1), Pico SDK 2.3.1, macOS host. If #125 changes a fault bit, re-measure on the merged commit and add a row; do not edit this one.

Section-4 command in `build_make/`: `make clean`, then `make -k -j8 SMP=1 CONSOLE=usb_cdc WIFI=none NET=none MQTT=0 CRASH_REPORT=0 TEST=0`, then `arm-none-eabi-size mtk3pico_smp1_usb_cdc.elf`. Result: 0 warnings, 0 undefined references.

| Build | text | data | bss |
| --- | ---: | ---: | ---: |
| Final implementation (`caa7f8c`) | 72272 | 552 | 23716 |

Probes: each `testing/subsystemN/sizeof_probe.c` compiled with the flags in `testing/subsystem3/sizeof_probe.sh` plus `-Iinclude -Iconfig -Iapp_program`. Linked objects: `arm-none-eabi-nm -S -t d` on the same build. Instances count persistent copies only; task-local copies live on task stacks and are exempt.

| Subsystem | Type | `sizeof` (bytes) | Linked object | Object size (bytes) | Instances | Match |
| --- | --- | ---: | --- | ---: | ---: | --- |
| S1 | `s1_event_log_t` | 168 | `event_log` (`subsystem1.o`) | 168 | 1 | yes |
| S2 | `MotionCommand` | 12 | `latest` (`subsystem2.o`) | 12 | 1 | yes |
| S2 | `MotionHealth` | 12 | `progress` (`subsystem2.o`) | 12 | 1 | yes |
| S3 | `S3IrSnapshot` | 40 | `latest` (`subsystem3.o`) | 40 | 1 | yes |
| S3 | `barcode_edge_t` | 6 | `ring` (`drv_barcode.o`) | 384 | 64 | yes (64 x 6) |
| S3 | `barcode_event_t` | 6 | `events` (`barcode.o`) | 48 | 8 | yes (8 x 6) |
| S3 | `barcode_frame_t` | 156 | `frame` (`barcode.o`) | 156 | 1 | yes |
| S4 | `TerrainContext` | 32 | `s_ctx` (`subsystem4.o`) | 32 | 1 | yes |
| S5 | `ObstacleProfile` | 14 | `profile` (`subsystem5.o`) | 14 | 1 | yes |

Message sizes: `testing/pure/test_mqtt_frame.c` encodes and decodes every golden frame at these lengths and passes (`mqtt_frame: PASS (telemetry 23 bytes)`); `scripts/mqtt_client.py --self-test` checks the same vectors.

| Frame | Bytes (`mqtt_frame.h`) |
| --- | ---: |
| Telemetry / snapshot | 23 |
| Event | 11 |
| Mapping | 5 |
| ACK | 3 |
| Snapshot request | 2 |

No comparison with the original implementation is required (course update 2026-10-27). Timing (T15) and physical behaviour are not covered here; see #117.
