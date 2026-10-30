# System and physical test log

## Contents

- [1. Test cases](#1-test-cases)
- [2. Results](#2-results)
- [3. Final memory and message size (SYS-11)](#3-final-memory-and-message-size-sys-11)
- [Current resource audit (SYS-12)](#current-resource-audit-sys-12)
- [Documentation alignment — 2026-10-29](#documentation-alignment--2026-10-29)

Retired template definitions: SYS-01 illustrative ten-second WiFi reading; SYS-02 unfilled manual placeholder. No template result was recorded. Full mission report trials T01-T16 remain pending. No source commit or hardware result is fabricated.

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

## Documentation alignment — 2026-10-29

Planning note only; no test was executed and no historical result is changed.
The [central plan](../../plan.md) owns future implementation sequence;
[mission-readiness](../mission-readiness.md) owns target physical procedures.
Earlier task splits, GP20 reset flows, JSON messages, watchdog assumptions and
configured timing targets describe their recorded revisions. Do not interpret
them as target passes. Use the applicable procedure revision, exact source and
new evidence filenames for reruns; never overwrite evidence or reuse retired IDs.

2026-10-29 availability note: the user confirmed that hardware trials are
unavailable in this session. The pin allocation is user-confirmed through
team.md, but no continuity, rail, polarity, calibration, timing, bypass or
full-course trial was executed. All corresponding current-image physical
acceptance remains pending; [INT-17](../integration/logs/2026-10-29_INT-17_completion.txt)
records the full-plan BLOCKED audit. This is not a physical PASS or FAIL.

## Current resource audit (SYS-12)

Procedure defined 2026-10-30: perform the INT-24 host/four-profile build and
Cortex-M0+ probe procedure; save exact base HEAD plus working-source manifest,
compiler/SDK versions, ELF text/data/bss and image hashes. Cross-check persistent
symbols against target layouts. Record inter-task data copies separately from
MQTT frames. Expected frame lengths remain 23/11/5/3/2 bytes. This extends the
pre-restructure SYS-11 record without replacing or reusing it. Static allocated
RAM is data+bss; it is not measured live RAM, stack margin or heap high-water.
Final accepted-commit rerun and T15 timing remain pending.

2026-10-30 result: **PASS (host/build/static only)**, base commit
`62ee5226e16a3c501d94998291944753c9662dfc` plus the exact
[working-source manifest](../integration/logs/2026-10-30_INT-24-accepted-manifest.json).
Measured with Arm GNU 13.2.Rel1 (GCC 13.2.1), Pico SDK 2.2.0, macOS host;
SMP=1, CONSOLE=usb_cdc, WIFI=cyw43, CRASH_REPORT=0. Offline means NET=none,
MQTT=0 (radio driver remains linked); wireless means NET=lwip, MQTT=1.
TEST=0/1 selects normal/TEST. This differs from SYS-11's WIFI=none/toolchain;
these numbers do not establish a restructure-only saving.

| Profile | text | data | bss | Static allocated RAM (data+bss) |
| --- | ---: | ---: | ---: | ---: |
| offline-test | 323840 | 384 | 26456 | 26840 |
| offline-normal | 321756 | 384 | 26448 | 26832 |
| wireless-test | 378688 | 392 | 84128 | 84520 |
| wireless-normal | 376608 | 392 | 84120 | 84512 |

[Machine-readable sizes and 24 persistent object cross-checks](../integration/logs/2026-10-30_INT-24-accepted-resources.json)
match the [target probes](../integration/logs/2026-10-30_INT-24-accepted-abi.txt)
and per-profile `size`, symbol and image-SHA files under `integration/logs/`
(prefix `2026-10-30_INT-24-accepted-`). The existing configured three 2048-byte
stacks are requested allocations, not measured free stack. Runtime heap/stack
high-water marks, peak RAM and T15 remain pending.

| Copy / storage contract | Target bytes | Persistent instances / linked storage |
| --- | ---: | --- |
| Mission -> Motion command | 12 | one `latest`, 12 bytes |
| Motion -> readers health | 12 | one `progress`, 12 bytes |
| S3 -> readers snapshot | 40 | one `latest`, 40 bytes |
| Terrain -> readers `S4Snapshot` | 20 | two: `snapshot` and I/O-private `latest.0`, 40 bytes total |
| S5 range sample | 16 | two Mission samples + two reporting mirrors, 64 bytes total |
| S5 profile snapshot | 24 | composed copy: 14-byte profile + 6-byte evidence + 4-byte sampled time; not an additional persistent 24-byte object |
| S5 range group | 12 | embedded in 100-byte acquisition object |
| S5 ray | 4 | nine entries (36 bytes) embedded in acquisition object |
| S1 event log / event entry | 168 / 10 | one log with 16 packed entries |
| Mapping request / reply word | 4 / 4 | one each; five 4-byte tickets/counters plus one-byte sequence separately |
| Terrain published / startup word | 4 / 4 | one each, separate from the snapshots |
| Kernel TX / RX bridge | 104 / 56 | one each; slots include length and frame capacity, with head/tail words |
| Mission / recovery / bypass state | 168 / 20 / 84 | Mission is task-local; recovery and bypass embedded in it, not independent BSS objects |

Local codec adapter structures measure telemetry 30, event 10, mapping 5 and
ACK 2 bytes. They are never transmitted as native structs. The
[host golden-vector checks](../integration/logs/2026-10-30_INT-24-accepted-host.txt)
reconfirm MQTT frame lengths **23/11/5/3/2 bytes**, including the unchanged
snapshot body. No extra generic task-message queue is inferred from these sizes.

No firmware was executed/flashed. No-lost-functionality physical acceptance,
stop latency, WCET/jitter, ISR timing, missed deadlines, ring occupancy and stack
margin remain unmeasured. Re-run on the accepted committed image; complete the
mission-readiness checklist and T15 before claiming final product acceptance.

2026-10-30 subsequent working-tree refresh, INT-25: the bounded event-lock fix
adds two single-writer 32-bit diagnostic counters (8 bytes total). Fresh builds,
target probes and [25 persistent object cross-checks](../integration/logs/2026-10-30_INT-25-accepted-resources.json)
pass at the [new manifest](../integration/logs/2026-10-30_INT-25-accepted-manifest.json).
The INT-24 records above remain historical and are not overwritten.

| Profile | text | data | bss | Static allocated RAM (data+bss) |
| --- | ---: | ---: | ---: | ---: |
| offline-test | 323880 | 384 | 26464 | 26848 |
| offline-normal | 321796 | 384 | 26456 | 26840 |
| wireless-test | 378728 | 392 | 84136 | 84528 |
| wireless-normal | 376648 | 392 | 84128 | 84520 |

No firmware execution, lock-contention timing, live memory or physical pass.
