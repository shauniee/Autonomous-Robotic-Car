# Terrain (S4)

## Contents

- [Scope](#scope)
- [Files](#files)
- [Behaviour](#behaviour)
- [Contract](#contract)
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
- **Humps:** climb, crest, descent and complete-window validation, independent of Mission states. Mission requests half tracking speed during a fresh calibrated encounter.
- **Results:** only a valid completed encounter updates count or maximum, and results persist through starts and stops until reboot. Missing travel invalidates the current window but keeps completed results. Any failed quality sample disqualifies that encounter. Never fabricate height when data is missing, stale or dominated by acceleration.

## Contract

- `terrain_step` and `terrain_copy` publish acquisition and progress age beside packed results; unchanged words do not prove sensor progress. `terrain_read_word` gives the packed word.
- Use measured encoder travel with a private origin and modular subtraction. Never reset shared counts or substitute nominal travel.
- Publish impact evidence for Motion and Mission through shared data; never call a core-1 stop API across cores. A short impact must not disappear before it is seen.
- Bound I2C and each processing step, and never hold shared locks through I2C.

Widths: [data table](../../docs/project-specs.md#5-data-representation-table-week-13). Fault policy: [fault bitmask](../../docs/architecture.md#fault-bitmask). The 1 mm/count reported scale stays unless measurements justify a change.

## Status and open items

Algorithms pass synthetic replays; results are in the [terrain test log](../../testing/terrain/TEST_LOG.md). Physical height and impact calibration are not established.

- D06, D13 and D14: measured references up to 30 mm, height-error target, threshold and filter decisions.
- Test stationary, acceleration, braking, turning, climb, descent, impact, repeated reference passes, partial encounters and invalid-after-valid maximum (MR-16, MR-23, MR-29, T06, T07).
- Investigate the detector staying on for a whole run (INT-122).
