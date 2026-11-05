# Line and barcode

## Contents

- [Scope](#scope)
- [Files](#files)
- [Line tracking](#line-tracking)
- [Barcode](#barcode)
- [Where it stands](#where-it-stands)
- [Open issues](#open-issues)
- [Line success measure](#line-success-measure)
- [How to run a trial](#how-to-run-a-trial)
- [Lessons from the trials](#lessons-from-the-trials)

## Scope

Owner: Nicholas, who also owns Mission integration (`app_program/mission/`). line_barcode reads the two line sensors (GP26, GP27) and the barcode sensor (GP16), and publishes what they saw. There is no line_barcode task and line_barcode never writes motors. Mission owns steering, junctions, recovery and the [DOUT navigation contract](../../docs/architecture.md#dout-navigation-contract). Values live in [commissioning](../../docs/project-specs.md#dout-navigation-commissioning); wiring in [team](../../docs/team.md#2-pins).

## Files

| File | Job |
| --- | --- |
| `line_barcode.h` | The subsystem's only header |
| `line.c` | DOUT acquisition and classification |
| `barcode.c` | Edge capture, Code 39 decode, mapping and events |
| `line_barcode.c` | `line_barcode_step` task work and the line snapshot |

## Line tracking

- Both sensors come from one GPIO snapshot every `IR_SAMPLE_MS`; acquisition time is preserved and invalid or stale readings are rejected. There is no ADC or software IR calibration.
- Disconnect AOUT. `IR_BLACK_LEVEL` is 1: black reads HIGH.
- With drive power off, hold each mounted sensor at operating height and set its comparator until DOUT is steadily 0 on white and 1 on black. The module LED alone proves nothing. Expect white/white 0/0, left-only 1/0, right-only 0/1, broad black 1/1.
- For uncertain readings use `./flash.sh test`: motors stay inhibited and the USB console prints `[IR] ... digital L=... R=... SENSOR_ONLY`.
- White/white cannot tell a centred line from a lost one, so Mission's drift and loss distance resolve it. `line_ir_copy` copies under a private mutex; a successful copy does not mean fresh sensing.

## Barcode

- All A-Z decode as the same letter in either direction. Four distinct action letters arrive over MQTT; a missing mapping blocks start.
- Widths come from encoder displacement, not time. Widths use the sensor's own travel, weighted for its position left of the axle. After every black run the last 25 are tried as the last 5 elements of the first guard, a gap, the letter, a gap and the last guard, so no quiet zone is needed and a hump that corrupts the start of the first guard does not lose the read (INT-155). In each character the two widest bars and the widest space are wide. Runs under `BARCODE_MIN_ELEMENT_COUNTS` are chatter and merge into their neighbours. Backward wheel travel clears the runs, and each print yields one event.
- The ISR captures level and wheel counts through the shared core-1 GPIO dispatcher and returns; decoding is budgeted per release. An edge less than the shortest element (`BARCODE_MIN_ELEMENT_COUNTS`, about 1.4 mm of sensor travel) after one still waiting in the ring cancels it, so chatter takes no ring slots. Capacities: 64 edges, 8 events. Overflow is explicit and never replays an older action.
- `make ... BARCODE_DIAGNOSTICS=1` stores a 32-entry ring of edge positions, widths, result, orientation, letter and action, printed at most one per I/O period. Results: 0 none, 1 decoded, 2 bad config, 3 bad width, 4 bad frame, 5 bad symbol, 6 reversed travel. Logging can perturb timing.
- Polarity, element limits and sensor position are provisional (D01, D03, D04), with no physical read yet (INT-137).

## Where it stands

- **Last physical result (INT-123, the older PI law):** straights, three 90 degree curves and the plain crossing held at 100 mm/s; the junction after the three curves failed.
- **Source now, host and build only (INT-124 to INT-129; PI 2026-11-05):** distance-based PI steering, junction versus curve by black-onset distance, line loss past the last touch (`LINE_LOST_MM`), black evidence required for recovery and bypass return, pivot turns that end on the line, and a fixed 100 mm/s course speed. None of this has run on the car.
- **Hardware:** two DOUT modules 30 mm apart, outside both line edges, pair centre 120 mm ahead of the axle; barcode sensor about 185 mm forward, 55 mm left. Effective spot spacing 33 mm and line width 23 mm were measured before the last move (INT-98).
- `./flash.sh` releases turns and bypass. No physical pass exists for barcode turns, obstacle bypass or repeat runs across battery levels.

## Open issues

1. **Physical run of the new law.** Check the three-curve junction and weaving at the stronger edge correction. Set `NAV_ONSET_SKEW_MM` from `onset_skew_mm` in line traces.
2. **First line-trace capture.** Needs Comms owner agreement on the [line trace](../../docs/architecture.md#line-trace-17-bytes).
3. **Turn commissioning.** Measure the advance landing point, pivot coast at 100 mm/s and hand-over to FOLLOW.
4. **Loss and recovery.** Confirm no false loss at 350 mm (INT-135), and that the sweep finds the line from a real loss.
5. **Hump detector stuck on (INT-122, INT-133).** It no longer changes speed, but a stuck encounter can still miss hump results; cause not investigated in Terrain.
6. **Left IR clears black late (INT-192).** Not removed by the potentiometer; a module swap and a slow hand test will separate the module from its mounting.
7. **Values to measure:** tightest curve radius, effective track width, spot spacing at the 120 mm position, left-sensor span and hysteresis.

## Line success measure

Agreed by Nicholas on 2026-11-03 for M1. The 95% target is the user's.

- **Attempt:** one lap of a written course layout, from a fresh GP20 start at 100 mm/s, on one image with no changes between laps. The lap includes barcodes, junctions and humps.
- **Success, all of these:**
  - the car finishes the lap on the line with no hand contact, no `REACQUIRE` and no stop
  - each barcode reads as the printed letter
  - each junction takes the correct action, once
  - `humps=` equals the crossings, and each height is within the agreed limit (D10, not yet set)
- **Failure:** anything else, including a lap stopped for safety. Only a setup fault with a written reason may be left out, and it is still recorded.
- **Rate:** at least 20 laps per image and lighting condition, scored by `scripts/mission_trial_score.py`, with `TRACE=1` and `TRACE=0` scored apart.

## How to run a trial

1. Change one value in `app_program/common/robot_config.h`.
2. Build and flash: `./flash.sh trial`.
3. Copy `build_make/trial.uf2` to a named file, restore the source, and record the hash the script prints.
4. Run on battery with `mqtt_client.py monitor` saving to a file; analyse `WHEEL` and `LINE` records.
5. Run `testing/pure/run.sh` before changing any default; append a new INT entry.

## Lessons from the trials

- Fix the wheels before tuning steering: oversized feedforward looked like "steering too strong" (INT-99).
- Reducing delay (10 ms IR) and moving the sensors closer to the axle helped at 100 mm/s (INT-115, INT-117).
- Time-based exit rules fixed one case and moved the problem (INT-119 to INT-122).
