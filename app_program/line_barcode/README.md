# Line and barcode (S3)

## Contents

- [Scope](#scope)
- [Files](#files)
- [Line tracking](#line-tracking)
- [Barcode](#barcode)
- [Where it stands](#where-it-stands)
- [Open issues](#open-issues)
- [How to run a trial](#how-to-run-a-trial)
- [Lessons from the trials](#lessons-from-the-trials)

## Scope

Owner: Nicholas, who also owns Mission integration (`app_program/mission/`). S3 reads the two line sensors (GP26, GP27) and the barcode sensor (GP16), and publishes what they saw. There is no S3 task and S3 never writes motors. Mission owns steering, junctions, recovery and the [DOUT navigation contract](../../docs/architecture.md#dout-navigation-contract). Values live in [commissioning](../../docs/project-specs.md#dout-navigation-commissioning); wiring in [team](../../docs/team.md#2-pins).

## Files

| File | Job |
| --- | --- |
| `line.c`, `line.h` | DOUT acquisition and classification |
| `barcode.c`, `barcode.h` | Edge capture, Code 39 decode, mapping and events |
| `line_barcode.c`, `line_barcode.h` | `line_barcode_step` task work and the line snapshot |

## Line tracking

- Both sensors come from one GPIO snapshot every `IR_SAMPLE_MS`; acquisition time is preserved and invalid or stale readings are rejected. There is no ADC or software IR calibration.
- Disconnect AOUT. `IR_BLACK_LEVEL` is 1: black reads HIGH.
- With drive power off, hold each mounted sensor at operating height and set its comparator until DOUT is steadily 0 on white and 1 on black. The module LED alone proves nothing. Expect white/white 0/0, left-only 1/0, right-only 0/1, broad black 1/1.
- For uncertain readings use `./flash.sh test`: motors stay inhibited and the USB console prints `[IR] ... digital L=... R=... SENSOR_ONLY`.
- White/white cannot tell a centred line from a lost one, so Mission's drift and loss distance resolve it. `line_ir_copy` copies under a private mutex; a successful copy does not mean fresh sensing.

## Barcode

- All A-Z decode as the same letter in either direction. Four distinct action letters arrive over MQTT; a missing mapping blocks start.
- Widths come from encoder displacement, not time. Enabling capture queues the current level so the white approach can serve as the quiet zone. Backward wheel travel rejects the frame, and each encounter yields at most one action.
- The ISR captures level and wheel counts through the shared core-1 GPIO dispatcher and returns; decoding is budgeted per release. Capacities: 64 edges, 8 events. Overflow is explicit and never replays an older action.
- `make ... BARCODE_DIAGNOSTICS=1` stores a 32-entry ring of edge positions, widths, result, orientation, letter and action, printed at most one per I/O period. Results: 0 none, 1 decoded, 2 bad config, 3 bad width, 4 bad frame, 5 bad symbol, 6 reversed travel. Logging can perturb timing.
- Polarity, narrow/wide bounds, quiet zone, sensor position and spacing are provisional (D01, D03, D04). Offset compensation is deferred until captures show distortion.

## Where it stands

- **Last physical result (INT-123, the older PI law):** straights, three 90 degree curves and the plain crossing held at 100 mm/s; the junction after the three curves failed.
- **Source now, host and build only (INT-124 to INT-129):** distance-based steering, junction versus curve by black-onset distance, line loss 200 mm past the last touch, black evidence required for recovery and bypass return, pivot turns that end on the line, and a fixed 100 mm/s course speed. None of this has run on the car.
- **Hardware:** two DOUT modules 30 mm apart, outside both line edges, pair centre 120 mm ahead of the axle; barcode sensor about 185 mm forward, 55 mm left. Effective spot spacing 33 mm and line width 23 mm were measured before the last move (INT-98).
- `./flash.sh` releases turns and bypass. No physical pass exists for barcode turns, obstacle bypass or repeat runs across battery levels.

## Open issues

1. **Physical run of the new law.** Check the three-curve junction and weaving at the stronger edge correction. Set `NAV_ONSET_SKEW_MM` from `onset_skew_mm` in line traces.
2. **First line-trace capture.** Needs S1 owner agreement on the [line trace](../../docs/architecture.md#line-trace-17-bytes).
3. **Turn commissioning.** Measure the advance landing point, pivot coast at 100 mm/s and hand-over to FOLLOW.
4. **Loss and recovery.** Confirm the drift touches well inside 200 mm, and that the sweep finds the line from there.
5. **Hump detector stuck on (INT-122).** Speed halved for a whole run; cleared by an IMU check and restart; cause not investigated in S4.
6. **Values to measure:** tightest curve radius, effective track width, spot spacing at the 120 mm position, left-sensor span and hysteresis.

## How to run a trial

1. Change one value in `app_program/common/robot_config.h`.
2. Build and flash: `./flash.sh`.
3. Copy `build_make/mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_course.uf2` to a named file, restore the source, and record the hash the script prints.
4. Run on battery with `mqtt_client.py monitor` saving to a file; analyse `WHEEL` and `LINE` records.
5. Run `testing/pure/run.sh` before changing any default; append a new INT entry.

## Lessons from the trials

- Fix the wheels before tuning steering: oversized feedforward looked like "steering too strong" (INT-99).
- Reducing delay (10 ms IR) and moving the sensors closer to the axle helped at 100 mm/s (INT-115, INT-117).
- Time-based exit rules fixed one case and moved the problem (INT-119 to INT-122).
