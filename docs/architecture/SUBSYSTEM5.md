# Subsystem 5: Obstacle

Owner: Jia Jing. S5 owns ultrasonic Echo capture, servo acquisition and stationary profiles. Mission owns avoidance movement through S2. [TEAM](../../TEAM.md) owns pins/PWM; [TECHNICAL_INFO](../../TECHNICAL_INFO.md#ultrasonic-sensor) owns variant/electrical facts.

## Continuous acquisition and scanning

TRIG/Echo acquisition is continuous and bounded, with a 30 ms response window. Missing, stuck, late or out-of-range Echo is unknown, never clear space. TEST can expose raw Echo timing while configured range validity is false. Servo signal stays inhibited until calibrated settings qualify.

A scan requires fresh measured stationarity, unique scan ID and absolute deadline. Five coarse rays span -60,-30,0,30,60 degrees; four fine rays focus around the nearest coarse return within the supported span. Requested pulse/angle mapping, center/endpoints and settling require actual measurements.

Forward and profile timestamps/validity are separate. During a scan the forward reading is unavailable for motion. Afterward restore center, wait for settling and acquire a fresh forward return before completion/movement. Cancellation dominates deadline/movement failure and remains Cancelled through center restoration; it cannot become Done on a later center Echo.

## Profile and Mission boundary

Profile coordinates include measured forward/lateral sonar mounting offsets and are relative to the car center. Closest radius is compared with the swept pivot envelope. Width and side clearance are supported estimates from valid rays, not proof that all space between rays is free.

Mission brakes before the pivot envelope using measured worst-case headroom for sensing/control latency and stopping. It selects greater supported side clearance, left on a tie, and performs a bounded encoder dogleg with confirmed departure/depth bounds. Reacquisition needs expected position/heading/corridor and fresh line observations. Unknown/blocked/ambiguous layouts stop. A nested obstacle is inspected while braked then stops as an unsupported retry.

Center-only moving sensing does not observe the adjacent/rear edge. The depth-bound design needs explicit course acceptance; do not claim passed-edge observation. Sonar/servo calibration, body envelope, braking headroom and both-side/wrong-line trials remain physical gates.

Even a completed valid profile does not release the brake until Mission revalidates every required producer and measured stationarity. [Architecture](../ARCHITECTURE.md#safety-and-acceptance) owns this resume gate.

## Interfaces and verification

`ObstacleSnapshot` contains independent forward/profile times, scan state/ID, rays and profile validity. `s5_obstacle_copy`, `s5_scan_request`, `s5_scan_cancel` and reset are bounded task APIs. `s5_profile_build_config` and `s5_scan_guard` are the live configuration/phase predicates exercised by fixtures.

Modules: lifecycle/task `subsystem5` and `obstacle_profile`. [S5 cases](../../testing/subsystem5/TEST_LOG.md), T08-T10 and [integration](../../testing/integration/TEST_LOG.md) own evidence. Scan publication cannot override GP21/Safety.
