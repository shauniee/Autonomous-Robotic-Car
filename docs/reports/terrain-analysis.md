# Terrain and hump analysis — evidence worksheet

**Status:** Draft analysis and physical measurement protocol, not a completed
experiment. Owner: S4 (Terrain). Requirements M4/M5, MR-11/16/23/29, D06/D10/D11.

## Current implementation

`app_program/terrain/terrain_algo.c` calibrates the GY-511 accelerometer
using 100 stationary samples. The running filter stores calibrated
acceleration in Q16 scaled units and computes pitch and roll. For the
single-wheel hump case, the detector compares roll with a baseline updated
only while stationary, rather than following a moving bend. It qualifies a
hump by the configured roll entry/exit thresholds, plausible height,
wheel travel and a run of flat exit samples. An incomplete or invalidated
window does not increment the completed count.

Provisional values in `robot_config.h` include
`HUMP_ROLL_ENTER_DEG=3.5`, `HUMP_ROLL_EXIT_DEG=2.0`,
`HUMP_MIN_TRAVEL_MM=50`, `HUMP_MAX_TRAVEL_MM=400`,
`HUMP_CALIB_FACTOR=0.8` and `HUMP_FLAT_SAMPLES=10`.
These were **not** accepted as final physical calibration.
Impact detection is a separate condition and must request a latched safety
stop when confirmed.

## Measurements to collect

| Scenario | Trials | Measured reference | Record | Decision |
| --- | --- | --- | --- | --- |
| Level standstill/startup | At least 3 boots | Stationary and tilted setup | IMU calibration completion, baseline roll, false events | D06 |
| Single known 10 mm hump | At least 3 passes | Height from calipers | Completed count, reported height, profile, any double count | MR-29 |
| Additional reference up to 30 mm | At least 3 passes per height | Known true height | Actual versus reported, error per pass | D06/D10 |
| Pure pivot or tight curve | Repeated | No hump | Event count must not increase | INT-158 |
| Hump before barcode / on curve | Repeated | Known route | Event count and barcode/line continuation | MR-29 |
| One-wheel and reversed approach | Repeated | Known wheel and direction | Roll sign, height, count and completion | D11 |
| Sharp chassis tap | Repeated safe test | Controlled impact | Fault byte, braked output and persistent stop | MR-16 |
| IMU disconnected | Bench only, power off first | No device | Bounded calibration failure and fault policy | MR-11 |

The earlier 5-humps-for-1-crossing observation and the later software fix
must be checked on the same physical course. Do not change the height factor,
roll thresholds or minimum travel based on a single noisy observation.

## Analysis and decision

For each reference height, record individual signed error
`reported_height_mm - measured_height_mm`; then calculate mean absolute
error and maximum absolute error only from recorded measurements.
First obtain the acceptable error bound from the team/instructor (D10).
Check whether impact stops, network telemetry and incomplete hump windows
behave as separately specified. Record build SHA, image hash, mounting
orientation, speed, calibration pose, battery and raw captures.

## Results

**Pending physical trials and owner sign-off.** Host algorithm tests and
previous anecdotal passes do not establish the true height accuracy or
reliable single-event counting on the actual robot.
