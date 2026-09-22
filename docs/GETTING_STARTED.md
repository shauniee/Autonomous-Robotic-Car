# How to start this project

## Understand the assignment

The briefing describes a single autonomous car with five cooperating subsystems.
It must follow a line, interpret navigation barcodes, detect and measure humps,
profile and bypass obstacles, reacquire the line, and publish status/telemetry.
The vehicle controller coordinates those decisions and sends motion targets.

The assessment covers mission completion, navigation accuracy, perception,
robustness, software quality/efficiency, integration/teamwork, and demonstration
evidence. The brief mentions a Week 10 demonstration but does not give a detailed
weekly schedule or numerical pass thresholds. The sequence below is a proposed
team plan, not an additional course requirement.

## First team session

1. Confirm the exact board: Pico, Pico W, or another variant. Standard Pico needs
   an external WiFi solution to satisfy the communication requirement.
2. List the actual motor driver, encoder, IR sensor, IMU, ultrasonic sensor,
   servo, battery and regulator part numbers in [HARDWARE.md](HARDWARE.md).
3. Assign each buddy a subsystem and name an integration owner for `controller.c`.
   Agree on [shared units, event meanings and API ownership](ARCHITECTURE.md).
4. Run `scripts/check.ps1` and read the eight demo status lines. Follow the
   control flow from `sim/main.c` into `controller_step()`.
5. Set up the Pico SDK. Build and flash the starter. Verify USB status and the
   `s`/`x` bench commands. With the current invalid sensor stubs, `s` should report
   a sensor fault and `x` should report a commanded stop.
   For Buddy 1, follow [WiFi setup](WIFI_TELEMETRY.md) to configure the implemented
   Pico W transport and check telemetry independently of the sensor drivers.
6. Agree on measurable success targets and start a calibration/test log.

Do not begin with five complete programs that each own `main()`. Each buddy
implements their module, while the team maintains one application loop.

## Build in this order

| Stage | Work | Evidence before moving on |
| --- | --- | --- |
| 1. Board bring-up | USB output, confirmed pin map, one sensor reading at a time | Repeatable raw readings and correct timestamps |
| 2. Motion | Driver stop/enable, wheel direction, encoder counts, speed/distance, per-wheel PID | Both wheels stop reliably; logged commanded versus measured speed; measured distance/turn error |
| 3. Line following | Black/white calibration, normalised line error, low-speed steering, line-loss stop | Repeatable short line runs and controlled loss-of-line stop |
| 4. Basic integration | Fresh IR + IMU + forward ultrasonic readings into controller | Obstacle/collision/invalid-data stop tests with real readings |
| 5. Navigation and terrain | Barcode event decoder, bounded turns/junction handling, IMU filtering/hump measurement | Known barcode routes and measured hump trials |
| 6. Obstacle mission | Coarse/fine servo scanning, profile, bounded bypass, line reacquisition | Repeated bypass and recovery runs, including blocked/no-echo cases |
| 7. Full mission | MQTT telemetry/commands, reconnect handling, combined route | Repeatable end-to-end demonstration with logs and recovery evidence |

Buddy 1 can develop communications alongside stages 2-6; begin with USB logging
so network setup does not prevent bench measurements. Integrate small changes
throughout, and reserve time before Week 10 for repeated full-route rehearsals.

The first controller version deliberately requires valid line, terrain and
forward-range readings before moving. Run individual motor/sensor bench checks
in dedicated test programs as you add drivers; do not disguise missing sensor
drivers as valid readings in the production loop.

## First deliverable for each buddy

| Buddy | First useful increment | Eventual briefing deliverables |
| --- | --- | --- |
| 1: WiFi | USB status, agreed MQTT schema, board/network decision | Communication API, telemetry framework, MQTT documentation and demo |
| 2: Motion | Stop both motors; read wheel encoder counts | Motion library, PID tuning report and accuracy evaluation |
| 3: IR + barcode | Log three calibrated IR channels and line error | Line follower, barcode decoder and navigation command interface |
| 4: IMU | Stable timestamped raw readings and bias calibration | IMU processing, hump detection algorithm and terrain analysis report |
| 5: Ultrasonic + servo | Fixed forward distance with timeout handling | Adaptive scanner, obstacle profile, avoidance and recovery algorithms |

## Decisions still needed

- C is the starter choice; confirm any course-mandated language or RTOS.
- Confirm barcode encoding, line/junction geometry, obstacle rules, course
  dimensions and numerical assessment targets with your instructor.
- The implemented transport targets Pico W and a local IPv4 MQTT broker; fill the
  local configuration and confirm the module mounted on the carrier board.
- The current default stops on loss of an established MQTT connection. Agree with
  the team before changing `ROBOT_STOP_ON_DISCONNECT`.
- Set measured sensor rates, timeouts, speed limits and stopping clearance.
  Values in `config.h` are examples, not calibrated operating limits.
- Define how a navigation action completes, times out, and hands control back.

## Evidence log

Use [test-log.csv](test-log.csv) for each repeated trial. Record firmware revision,
setup, expected result, actual measurement, units, pass/fail and a log/photo link.
Set acceptance values before testing; leave unknown values blank until agreed.
Useful measurements include speed error, distance/turn error, line-run success,
barcode success across speeds, hump height error, obstacle width error, recovery
success and telemetry/reconnect behavior.
