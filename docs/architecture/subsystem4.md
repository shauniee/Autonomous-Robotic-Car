# Subsystem 4: Terrain and motion monitoring

## Contents

- [1. Functionality](#1-functionality)
- [2. States and responses](#2-states-and-responses)
- [3. Interfaces and contract](#3-interfaces-and-contract)
  - [3.1 Provided](#31-provided)
  - [3.2 Consumed from Motion](#32-consumed-from-motion)
  - [3.3 Safety path](#33-safety-path)
- [4. Data and fault bits](#4-data-and-fault-bits)
- [5. Status](#5-status)
- [6. Design rules](#6-design-rules)
- [7. Open items](#7-open-items)

## 1. Functionality

Owner Darrius. [Plan](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117) owns implementation sequence;
[architecture](../architecture.md) owns tasks/fault policy;
[team](../team.md#2-pins) owns wiring. Target Terrain is bounded work in core-0
`io_task`, not a separate core-1 task. Reuse GY-511 acceleration acquisition,
stationary calibration, filtering, hump processing and impact evidence.
The GY-511 has no gyroscope; label encoder-derived turn rate accurately.

## 2. States and responses

Boot: attempt mounted stationary IMU calibration before IR calibration, with a
bounded completion/failure handshake. Missing/failed calibration remains flagged
but permits IR calibration and mission readiness after the attempt. Normal IMU
failure is report-only; valid detected impact is a stopping cause. This differs
from current remote start-blocking masks. Do not fabricate pose/height when data
is missing, stale or dominated by acceleration.

The working boot attempt allows 5 seconds to collect 100 fresh stationary
samples at the configured 50 Hz sensor rate. This software budget includes
scheduling margin; it is not a measured hardware timing guarantee. Encoder
movement restarts the sample accumulation without extending the deadline.
USB console `[IMU]` messages distinguish I2C initialization failure, completion
and timeout (sample count and last acquisition result). A failed attempt is
retried only by rebooting, not by the local mission reset.

Hump processing is independent of top-level Mission states: climb, crest,
descent, complete-window validation. Mission requests half normal tracking speed;
settle at a steady reduced speed, filter pitch and account for acceleration/braking
transients. Only a valid completed encounter increments count or updates maximum.
Interrupted/incomplete estimates cannot overwrite a valid maximum. Completed
results persist through accepted starts/stops until reboot; see the accepted
lifecycle in [project specs](../project-specs.md#pending-decisions).

## 3. Interfaces and contract

### 3.1 Provided

Current `subsystem4.c/.h` exposes `s4_terrain_word`; `terrain_algo.c/.h` owns
calibration/filter/classification/height; `drv_gy511.c/.h` owns I2C. `s4_step` and
`s4_copy` now publish coherent acquisition/progress age beside packed results; unchanged words cannot
prove sensor progress. Publish validity/failure even if no successful sample.

### 3.2 Consumed from Motion

Use measured encoder travel with a private origin and modular subtraction;
never reset shared counts or substitute nominal travel for absent required
feedback. Calibrated geometry converts counts to distance. Turn rate/heading
from encoders must not be labelled gyro data. Sensor fusion remains a stretch
item pending a demonstrated need, not a new required framework.

### 3.3 Safety path

Terrain on core 0 publishes impact evidence for Motion/Mission to observe through
coherent shared data. Do not call a core-1-only runtime stop/reset API cross-core.
Report/stop observation must not depend on telemetry event delivery. Impact capture
persistence must prevent a short event disappearing before Motion sees it;
review its latch/clear lifetime during the stop-contract implementation.

## 4. Data and fault bits

[Specs](../project-specs.md#5-data-representation-table-week-13) owns heights,
scales, invalid codes and packed filter state. [Architecture](../architecture.md#fault-bitmask)
owns the single policy table. Preserve 1 mm/count reported scale unless measurements
justify changing it. Height estimate/validity, count and greatest valid peak are
separate; existing MQTT fields/events remain schema-compatible.

## 5. Status

Pinned remote main `5e6a45b` contains the I2C driver, boot calibration loop,
filtered accelerometer pitch, encoder-travel height integration, packed Terrain
context, task and event publication. Earlier owner reports/configuration values
are not accepted measurements. The working implementation runs bounded Terrain steps in core-0 I/O, publishes
boot completion/failure before IR calibration is enabled and leaves ordinary
IMU/calibration faults report-only. The driver checks new-data status and bounds
a whole init/read operation by its existing timeout. Pending data never renews
sample time. Missing travel invalidates the current height window, retaining
completed count/maximum. Mission requests half tracking speed on fresh calibrated
encounters. Completed results are retained through starts/stops until reboot.
Height acceptance now requires the provisional acceleration-norm and wheel-motion
quality settings in [project specs](../project-specs.md#pending-decisions), with
200 ms of steady fresh evidence before a climb. Any failed sample disqualifies
that whole encounter; classification/impact detection still run and completed
count/maximum remain intact. These software checks are not physical calibration.
See [S4 results](../../testing/subsystem4/TEST_LOG.md); physical height/impact
calibration is not inferred from a compiled algorithm or synthetic replay.

## 6. Design rules

Bound I2C, calibration and each processing step; timeout paths must fit an
agreed I/O budget with slack for reporting and kernel work. Never hold shared
locks through I2C. Calibrate mounted axes/bias and compare filtered-pitch times
encoder-travel height against measured single-wheel references <=30 mm. No
uncorrected double integration or nominal-distance fallback is accepted.
Cross-core Terrain/Mission event producers need the S1 synchronization redesign.

## 7. Open items

D06/D13/D14: measured references, height-error target, threshold/filter decisions,
startup calibration outcome, acquisition-age publication and results lifecycle.
Test stationary/acceleration/braking/turn/climb/descent/impact, repeated reference
passes, partial encounters and invalid-after-valid maximum; MR-16/23/29, T06/T07.
