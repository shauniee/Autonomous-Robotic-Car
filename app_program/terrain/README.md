# Terrain

## Contents

- [Scope](#scope)
- [Files](#files)
- [Behaviour](#behaviour)
- [Contract](#contract)
- [Raw USB capture](#raw-usb-capture)
- [Terrain analysis (draft)](#terrain-analysis-draft)
- [Status and open items](#status-and-open-items)

## Scope

Owner: Darrius. IMU acquisition, boot calibration, filtering, hump measurement and impact evidence, as bounded work in core-0 `io_task`. The GY-511 has no gyroscope, so label encoder-derived turn rate accurately. Wiring: [team](../../docs/team.md#2-pins).

## Files

| File | Job |
| --- | --- |
| `terrain.h` | The subsystem's only header |
| `terrain.c` | Step, packed word and GY-511 I2C driver |
| `terrain_algo.c` | Calibration, filter, classification and height (host-tested) |
| `terrain_test.c` | Firmware self-tests |

## Behaviour

- **Boot:** up to 5 seconds to collect 100 fresh stationary samples at 50 Hz. Encoder movement restarts the count without extending the deadline. The USB console's `[IMU]` messages distinguish init failure, completion and timeout. A failure is flagged and report-only, and only a reboot retries it.
- **Humps:** course humps lift one wheel, so an encounter is sideways tilt (roll) of either sign, measured from its average while both wheels are stopped so the boot pose does not matter, over `HUMP_ROLL_ENTER_DEG` until it stays under `HUMP_ROLL_EXIT_DEG` for `HUMP_FLAT_SAMPLES`. It counts with `HUMP_MIN_TRAVEL_MM` of travel over the exit level and a height, track width times the sine of peak roll, between `HUMP_MIN_HEIGHT_MM` and `HUMP_MAX_HEIGHT_MM`. Past `HUMP_MAX_TRAVEL_MM` the tilt becomes the new average (the 2026-11-02 runs). Independent of Mission states. Humps do not change the course speed.
- **Turn rate (FR-20):** the GY-511 has no gyroscope, so turn rate is encoder-derived, not measured by the IMU. The laptop tool computes (right speed - left speed) / `TRACK_WIDTH_MM` from the encoder counts in consecutive telemetry frames and shows it in degrees per second, positive left: `monitor` prints `turn=`, the dashboard shows Turn under Motor speed. No firmware or frame change; it inherits the telemetry arrival-time limits of the speed readout and the uncalibrated effective track width (D03). Host-tested in the `mqtt_decoder` self-test.
- **Results:** only a valid completed encounter updates count or maximum, and results persist through starts and stops until reboot. Missing travel invalidates the current window but keeps completed results. Any failed quality sample disqualifies that encounter. Never fabricate height when data is missing, stale or dominated by acceleration.

## Contract

- `terrain_step` publishes the packed word and `terrain_read_word` reads it. An unchanged word does not prove new acquisition.
- Use measured encoder travel with a private origin and modular subtraction. Never reset shared counts or substitute nominal travel.
- Publish impact evidence for Motion and Mission through shared data; never call a core-1 stop API across cores. A short impact must not disappear before it is seen.
- Bound I2C and each processing step, and never hold shared locks through I2C.

Widths: [data table](../../docs/team.md#6-data-representation). Fault policy: [fault bitmask](../../docs/architecture.md#fault-bitmask). The 1 mm/count reported scale stays unless measurements justify a change.

## Raw USB capture

Build with `TERRAIN_DIAGNOSTICS=1` to log every fresh accelerometer sample,
including calibration and flat ground, to USB as `[IMU_RAW]` CSV rows. The
default is `0`; the diagnostic image has an `_imudiag` suffix. USB output
does not wait for a host, but formatting adds work and USB congestion can
lose bytes. This mode is for investigation; its timing is not a normal-run
timing measurement.

For live readings, open the Pico's COM port in a serial monitor at 115200 baud.
No Python tool is needed. Columns: sequence, time_ms, raw_x, raw_y, raw_z,
pitch_cdeg, left_delta, right_delta, calibrated, state, encoder_valid,
roll_cdeg, stationary, height_valid, hump_count, rise_tenths_mm, usb_dropped.
Divide either tilt value by 100 to read degrees.

From the repository root, using the existing build environment:

```sh
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=1 TERRAIN_DIAGNOSTICS=1
```

Flash `build_make/mtk3pico_smp1_usb_cdc_wifi_test_imudiag.uf2` using the existing
flash procedure. `TEST=1` inhibits motors: roll the robot by hand over the
reference hump with both wheels turning forward; tilting it in place cannot
produce a valid height. For a powered trial, use the normal wireless build
flags with `TEST=0 TERRAIN_DIAGNOSTICS=1` and the existing power/run procedure.

Install `pyserial` on the laptop once, then capture using the actual port:

```sh
python -m pip install pyserial
python scripts/capture_imu.py --port COM5 --seconds 60 --output testing/terrain/logs/S4-U04/YYYY-MM-DD_hump-raw-01.log
```

Start capture before resetting the board. Keep it level with wheels still
through calibration, then cross the hump and return to flat ground. Close
other programs using that serial port. The script never writes commands to
the serial port and refuses to overwrite an existing CSV. To convert an
existing console log, replace `--port COM5 --seconds 60` with `--input log.txt`.
Exit code 1 means no samples, malformed rows, sequence gaps/resets, USB byte
loss or a capture error; inspect the output before accepting a trial.

Columns are defined by `scripts/capture_imu.py:FIELDS`. Raw X/Y/Z are signed
driver counts after unpacking the sensor's left-aligned 12-bit readings,
before calibration or filtering; multiply by `ACCEL_MG_PER_LSB` for g.
`pitch_cdeg` and `roll_cdeg` are filtered forward/backward and sideways tilt
in 0.01 degrees, meaningful after calibration. States: 0 flat, 1 on hump,
2 impact. Encoder deltas are counts since the previous fresh acquisition
(calibration/error resets that origin). `encoder_valid` reports encoder health;
`stationary` means both wheel counts were unchanged on this poll. These are
context, not a second set of hump acceptance rules. `hump_count` increasing
marks a completed accepted hump; `rise_tenths_mm` is intermediate rise before
the height calibration factor. `usb_dropped_bytes` is cumulative console loss.
Sequence gaps identify missing rows; timestamp gaps can reveal delayed
acquisition. The sequence restarts on reboot. Calibration completion does not
prove correct sensor mounting.

Record the flashed image hash, source revision, build flags, measured hump,
power and mounting with each physical trial in the terrain test log.

## Terrain analysis (draft)

Draft from existing evidence only. No reference height above one 10 mm hump has been measured.

- **Detector:** pitch did not show a one-wheel hump, so a 2026-11-02 change replaced it with roll.
- **Counting:** One 2026-11-02 run counted the wrong encounters, and another reported `humps=5` for 1 crossing because pivots moved the baseline. The fix passes host tests and has never run on the car.
- **Height:** one 10 mm hump read about 12.5 mm uncorrected, which gives `HUMP_CALIB_FACTOR` 0.8. That is one height, not a calibration.
- **Travel limit:** `HUMP_MIN_TRAVEL_MM` 50 rests on two crossings (76 and 156 mm) against curves of 26 mm or less.
- **Turn rate:** no turn-rate monitor exists in Terrain. The GY-511 has no gyroscope, so any monitor must be encoder-derived and labelled so (T06).

Missing before this is a report: reference humps up to 30 mm with an agreed error limit, labelled traces and an impact result.

### Measurements to collect

Record each run separately with source SHA, UF2 hash, setup and evidence. Limits stay pending until agreed.

| Scenario | Trials | Measured reference | Record | Decision |
| --- | --- | --- | --- | --- |
| Level standstill/startup | At least 3 boots | Stationary and tilted setup | IMU calibration completion, baseline roll, false events | D06 |
| Single known 10 mm hump | At least 3 passes | Height from calipers | Completed count, reported height, profile, any double count | MR-29 |
| Additional reference up to 30 mm | At least 3 passes per height | Known true height | Actual versus reported, error per pass | D06/D10 |
| Pure pivot or tight curve | Repeated | No hump | Event count must not increase | 2026-11-02 |
| Hump before barcode / on curve | Repeated | Known route | Event count and barcode/line continuation | MR-29 |
| One-wheel and reversed approach | Repeated | Known wheel and direction | Roll sign, height, count and completion | D11 |
| Sharp chassis tap | Repeated safe test | Controlled impact | Fault byte, braked output and persistent stop | MR-16 |
| IMU disconnected | Bench only, power off first | No device | Bounded calibration failure and fault policy | MR-11 |

## Status and open items

Algorithms pass synthetic replays; results are in the [terrain test log](../../testing/terrain/TEST_LOG.md). Physical height and impact calibration are not established.

- D06, D13 and D14: measured references up to 30 mm, height-error target, threshold and filter decisions.
- Test stationary, acceleration, braking, turning, climb, descent, impact, repeated reference passes, partial encounters and invalid-after-valid maximum (MR-16, MR-23, MR-29, T06, T07).
