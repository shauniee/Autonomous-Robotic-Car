# Terrain

## Contents

- [Scope](#scope)
- [Files](#files)
- [Behaviour](#behaviour)
- [Contract](#contract)
- [Terrain analysis (draft)](#terrain-analysis-draft)
- [Status and open items](#status-and-open-items)

## Scope

Owner: Darrius. IMU acquisition, boot calibration, filtering, hump measurement and impact evidence, as bounded work in core-0 `io_task`. The GY-511 has no gyroscope, so label encoder-derived turn rate accurately. Wiring: [team](../../docs/team.md#2-pins).

## Files

| File | Job |
| --- | --- |
| `terrain.h` | The subsystem's only header |
| `terrain.c` | Step, snapshot and GY-511 I2C driver |
| `terrain_algo.c` | Calibration, filter, classification and height (host-tested) |
| `terrain_test.c` | Firmware self-tests |

## Behaviour

- **Boot:** up to 5 seconds to collect 100 fresh stationary samples at 50 Hz. Encoder movement restarts the count without extending the deadline. The USB console's `[IMU]` messages distinguish init failure, completion and timeout. A failure is flagged and report-only, and only a reboot retries it.
- **Humps:** course humps lift one wheel, so an encounter is sideways tilt (roll) of either sign, measured from its average while both wheels are stopped so the boot pose does not matter, over `HUMP_ROLL_ENTER_DEG` until it stays under `HUMP_ROLL_EXIT_DEG` for `HUMP_FLAT_SAMPLES`. It counts with `HUMP_MIN_TRAVEL_MM` of travel over the exit level and a height, track width times the sine of peak roll, between `HUMP_MIN_HEIGHT_MM` and `HUMP_MAX_HEIGHT_MM`. Past `HUMP_MAX_TRAVEL_MM` the tilt becomes the new average (INT-156, INT-157). Independent of Mission states. Humps do not change the course speed.
- **Results:** only a valid completed encounter updates count or maximum, and results persist through starts and stops until reboot. Missing travel invalidates the current window but keeps completed results. Any failed quality sample disqualifies that encounter. Never fabricate height when data is missing, stale or dominated by acceleration.

## Contract

- `terrain_step` and `terrain_copy` publish acquisition and progress age beside packed results; unchanged words do not prove sensor progress. `terrain_read_word` gives the packed word.
- Use measured encoder travel with a private origin and modular subtraction. Never reset shared counts or substitute nominal travel.
- Publish impact evidence for Motion and Mission through shared data; never call a core-1 stop API across cores. A short impact must not disappear before it is seen.
- Bound I2C and each processing step, and never hold shared locks through I2C.

Widths: [data table](../../docs/project-specs.md#5-data-representation-table-week-13). Fault policy: [fault bitmask](../../docs/architecture.md#fault-bitmask). The 1 mm/count reported scale stays unless measurements justify a change.

## Terrain analysis (draft)

Draft from existing evidence only. No reference height above one 10 mm hump has been measured.

- **Detector:** pitch did not show a one-wheel hump, so INT-156 replaced it with roll.
- **Counting:** INT-157 counted the wrong encounters, and INT-158 reported `humps=5` for 1 crossing because pivots moved the baseline. The fix passes host tests and has never run on the car.
- **Height:** one 10 mm hump read about 12.5 mm uncorrected, which gives `HUMP_CALIB_FACTOR` 0.8. That is one height, not a calibration.
- **Travel limit:** `HUMP_MIN_TRAVEL_MM` 50 rests on two crossings (76 and 156 mm) against curves of 26 mm or less.
- **Turn rate:** no turn-rate monitor exists in Terrain. The GY-511 has no gyroscope, so any monitor must be encoder-derived and labelled so (T06).

Missing before this is a report: reference humps up to 30 mm with an agreed error limit, labelled traces and an impact result.

## Status and open items

Algorithms pass synthetic replays; results are in the [terrain test log](../../testing/terrain/TEST_LOG.md). Physical height and impact calibration are not established.

- D06, D13 and D14: measured references up to 30 mm, height-error target, threshold and filter decisions.
- Test stationary, acceleration, braking, turning, climb, descent, impact, repeated reference passes, partial encounters and invalid-after-valid maximum (MR-16, MR-23, MR-29, T06, T07).
