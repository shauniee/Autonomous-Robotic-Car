# System and physical test log

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

## 2. Results

| Date | ID | Revision | Result | Reason |
|---|---|---|---|---|
| 2026-10-08 | SYS-03/04/05/06 | To record before trial | PENDING | No connected Pico; physical calibration/tracking/stop/reset cannot be performed in this session |
| 2026-10-08 | SYS-07/08 | To record before trial | PENDING | Proportional refactor software checks only; barcode deferred, no physical trial performed |
| 2026-10-08 | SYS-09/10 | To record before trial | PENDING | MQTT profiles built and host/local broker checks passed; no Pico flash, radio connection or physical trial performed for this addition |

Do not connect USB while the power bank is connected. Use camera/scope/operator records during battery trials; USB logs are collected only in the separate USB-only phase. Record actual wiring, calibrated references and exact flashed firmware checksum/revision in every result.
