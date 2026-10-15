# Subsystem 4: Terrain

Owner: Darrius. S4 owns bounded GY-511 acquisition, mounted gravity/bias calibration, movement classification and reference-calibrated hump estimates. [TECHNICAL_INFO](../../TECHNICAL_INFO.md#gy-511--lsm303dlhc-imu) owns chip facts; [TEAM](../../TEAM.md) owns I2C allocation.

## Acquisition and calibration

The driver requests acceleration at 50 Hz, high-resolution +/-2 g with coherent block-data acquisition. Each I2C transaction has a 5 ms budget with abort/STOP checks. Magnetometer data uses its separate ready flag, timestamp and validity. GY-511 has no gyroscope; no gyro-derived rate or calibrated compass is claimed.

Calibration starts only on the S3 black-press request. `s4_calibration_request(id, deadline)` requires fresh raw encoder quiet counts and stopped/brake-acknowledged Motion; calibrated wheel geometry is not required. The task owns calibration buffers, matching cancellation and publication. Collect 100 distinct upright gravity samples; movement, bad gravity, stale/error data, GP21 cancellation or the five-second deadline invalidates the attempt. Repeated cached samples cannot count twice.

Snapshot calibration exposes ID/status/reason/sample count. S3 defers the first success beep and white capture until black+IMU both succeed. Ordinary stopping preserves accepted bias; hardware reinitialization requires a new explicit calibration.

## Movement and humps

`terrain_motion_measure` uses fresh distinct encoder sequences, unchanged run epoch and nonzero acquisition-time delta. Turning is independent of wheel-agreement filtering used for hump travel. Classification prioritizes impact, then valid stationarity/turning, climb/descent, acceleration and moving; unavailable derived fields carry invalidity rather than fabricated rates.

Hump states are Flat -> Climbing -> Cresting -> Descending -> validated baseline return. Height combines measured encoder travel with mounted pitch and a separately reference-fitted roll contribution for one-wheel encounters. Encoder-derived acceleration above the configured threshold, gravity disagreement, backward/unreliable travel, wheel disagreement, incomplete windows and deadlines reject candidates. Impact latches and independently requests S2 stop.

Accepted heights update last/max and an eight-record run history exactly on accepted baseline return, including equal-height encounters. Invalid later candidates do not replace the greatest accepted peak. Stop-only reset invalidates partial windows while preserving accepted data. New-run reset clears snapshot heights/count/history with the new epoch before WAIT/error acquisition can publish it. Reset requests retain all 32 epoch bits.

## Interfaces and verification

`TerrainSnapshot` contains acceleration, pitch/roll, separate magnetometer time/validity, encoder movement/rate validity, estimator state, last/max height, calibration and history. `s4_get_terrain` and publication are nonblocking coherent copies; requests are core-1 task handoffs, never ISR work.

Modules: `drv_gy511`, `terrain_algo`, `terrain_calibration`, `terrain_motion`, task and lifecycle `subsystem4`. Measured axes/signs, thresholds and reference height factors remain inputs, not firmware-generated calibration claims. [S4 tests](../../testing/subsystem4/TEST_LOG.md) cover fixtures; labelled movement/reference/one-wheel and acceleration rejection need physical evidence.
