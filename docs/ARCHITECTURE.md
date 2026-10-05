# Autonomous robotic car — system architecture

## Scope and status

This is the **target design** for the team car, based on [project-info.md](project-info.md), [tech-info.md](tech-info.md), and the [design review pin layout](<../../CSC01 Design Review.docx>). The car must autonomously follow a black line, decode navigation barcodes, execute left/right/straight/U-turn actions, detect and estimate the highest hump, profile and bypass obstacles, reacquire the original line, and report telemetry over WiFi/MQTT. It is a Pico WH on a Cytron Robo Pico carrier. On 5 October 2026 Nicholas requested swapping the IR Grove ports: right IR is now Grove 6 / GP27, left IR Grove 5 / GP26; the Word design is preserved as submitted. The table below uses the later bench IR swap; [HARDWARE.md](HARDWARE.md) also preserves the review’s original right/left allocation. Neither table proves installed electrical verification.

## Authority and implementation status

The [CSC01 Design Review](<../../CSC01 Design Review.docx>) is the primary integrated design reference. [Project information](project-info.md) retains assignment requirements and [technical reference](tech-info.md) retains datasheet evidence. See [the verification audit](DESIGN_REVIEW_AUDIT.md) for discrepancies and [hardware](HARDWARE.md) for submitted versus current bench wiring.

The review is a **proposed FreeRTOS design**, not implemented task scheduling or hardware-test evidence. The current `right_ir_follow_mqtt` application is a cooperative single-loop bench follower: right IR GP27/ADC1, M1/M2 PWM, local GP20/21, MQTT calibration/status and telemetry. It disables servo, omits IMU/sonar/encoders, and has no barcode, obstacle or hump mission. `robot_car` is the full-mission scaffold; the optional `sensor_bench_mqtt` is a stationary acquisition application. These are alternative firmware targets, not simultaneous programs on the robot.

## Target task boundaries and data flow

| Task / owner | Priority | Nominal release | Responsibility |
| --- | ---: | --- | --- |
| Safety / Nicholas | 5 | 10 ms and events | Local GP20/21; persistent stop/fault; independent wake-up of Motion |
| Motion / Mahesha | 4 | 20 ms and stop notification | Sole GP8–GP11 writer; encoder PID; action IDs, completion, command expiry and brake |
| Mission / Nicholas | 3 | 20 ms and bounded events | Mission arbitration; motion commands; scan requests; telemetry aggregation |
| Line/Barcode / Nicholas | 3 | ADC 20 ms; barcode IRQ | GP26/27 line observations; GP16 bar/space captures; one-shot navigation |
| Terrain / Darrius | 2 | 20 ms fresh polling | GY-511/I2C0; classification, impact and validated maximum hump height |
| Obstacle / Jia Jing | 2 | Bounded acquisition states | GP2 echo, GP3 trigger, GP12 servo; independent front/profile freshness |
| Communications / Shaun | 1 | Network events; 500 ms telemetry | WiFi/MQTT, status validation, bounded reconnection |

This allocation comes from review Sections 3–4. The current bench main directly owns motor GPIO; that is a limited bring-up implementation, not the proposed RTOS ownership split. The shared `include/robot/` interfaces still need action IDs, leases, U-turn and full RTOS contracts before mission integration.

Latest-value mailboxes `M_line`, `M_terrain`, `M_obstacle`, `M_motion_feedback` and `M_telemetry` hold coherent timestamped observations. `M_motion_cmd` carries ID-bound leased commands. FIFO `Q_nav` preserves one-shot events; `Q_scan_request` carries bounded scan requests. `Q_local` carries fresh local controls, `Q_status` requests reporting, `S_echo` wakes echo processing and `EG_system` carries Safety latches. Critical producer faults notify Safety independently of diagnostic queues. Snapshot overwrites must never erase navigation events or action completion; command expiry brakes locally. Queues, stack sizes, deadlines and lease bounds require measurement.

## Current intended wiring (bench IR override)

Grove port numbers are carrier connectors; GPIO names identify Pico signals. The two motor power-lead pairs connect **only** to the carrier's M1/M2 screw terminals, while their encoder wires connect to Grove ports. The Pico WH is plugged into the carrier socket; its physical header pins are not separate wires to add. Match any sensor adapter by its actual labels, not by a presumed cable position.

| Part | Signal or lead | Intended connection | Use / condition |
| --- | --- | --- | --- |
| Power bank | Red / black | Green carrier VIN `+` / `−` screw terminal | Measure voltage and polarity before powering. |
| Left motor | Yellow motor `−` / black motor `+` | M1B / M1A | Carrier drives M1 with GP8/GP9; verify which direction is physically forward. |
| Right motor | Yellow motor `−` / black motor `+` | M2B / M2A | Carrier drives M2 with GP10/GP11; verify forward direction. |
| Right encoder | Blue ground / white supply | Grove 1 GND / 3V3 | Verify encoder voltage and carrier 3V3 load. |
| Right encoder | Green B phase / red A phase | Grove 1 GP0 / GP1 | Two digital quadrature inputs; confirm A/B order and direction. |
| Left encoder | Blue ground / white supply | Grove 7 GND / 3V3 | Verify encoder voltage and carrier 3V3 load. |
| Left encoder | Green B phase / red A phase | Grove 7 GP7 / GP28 | Two digital quadrature inputs; GP28 cannot also sense battery voltage. |
| Right line IR | GND / VCC / AOUT | Grove 6 GND / 3V3 / GP27 | GP27 is ADC1; **DOUT is not connected**. |
| Left line IR | GND / VCC / AOUT | Grove 5 GND / 3V3 / GP26 | GP26 is ADC0; **DOUT is not connected**. |
| Barcode IR | GND / VCC / DOUT | Grove 4 GND / 3V3 / GP16 | Digital barcode edge input; **AOUT is not connected**. |
| HC-SR04P ultrasonic | GND / VCC / Echo / Trig | Grove 2 GND / 3V3 / GP2 / GP3 | Confirm 3.3 V-capable board and safe Echo HIGH before direct GP2 connection. |
| GY-511 three-axis module | GND / `Vin` / SDA / SCL | Grove 3 GND / 3V3 / GP4 / GP5 | I²C0; leave the breakout's separate `3.3` pin unconnected unless its circuit is verified. |
| SG90 servo | Orange / red / brown | Servo port 1 GP12 `S` / `+` / `−` | Check the actual lead colors, carrier silkscreen, and servo rail. |

Grove 5 and Grove 6 both expose **GP26**: use GP26 only for the left IR AOUT and leave Grove 6's GP26 contact unconnected. GP28 is the left encoder A input, so the carrier's optional GP28 battery-sense jumper must be open. Grove 2 shares GP2/GP3 with the Maker port, which is therefore unavailable for a second device on those pins. GP18, GP20/21, and GP22 serve onboard LED, buttons, and buzzer. Check available 3V3 current for all sensors, not merely each sensor in isolation.

### Electrical release conditions

1. **Confirm the HC-SR04P variant.** Nicholas reports an HC-SR04P, for which [a manufacturer lists 3–5.5 V operation](https://www.okystar.com/product-item/hc-sr04-ultrasonic-sensor-module-oky3261-2/38/). That makes the intended Grove 2 3V3 supply reasonable. Check the actual board marking, measure Echo HIGH at 3.3 V supply before connecting GP2, and verify GP3's trigger works. Direct Echo→GP2 is suitable only if the measured HIGH stays within the Pico's 3.3 V GPIO limit. The supplied 5 V HC-SR04 PDF describes a different variant; a 5 V-only replacement would require a different supply and Echo protection. A missing echo means *unknown*, never clear space.
2. Measure power-bank polarity and voltage at the carrier VIN input, 3V3 rail under load, servo rail, and the sensor output highs before attaching them to Pico GPIO. Pico GPIO/ADC inputs are 3.3 V devices. Confirm each encoder's 3.3 V supply and A/B output levels, IR AOUT range, and GY-511 SDA/SCL pull-up level. The carrier's motor limit is 1 A continuous and 1.5 A brief peak per channel; compare with measured motor current.
3. Confirm actual Grove connector contact order and sensor module header order. The port 5/6 GP26 overlap and port 7 GP28 battery-sense overlap make a full, unmodified four-wire cable potentially wrong. Keep wheels raised for initial powered motion and verify physical stop before track runs.

## Perception and control

- **Line and barcode:** Calibrate the analogue GP26/GP27 black/white ranges at the mounted height and demonstration lighting. Turn the two readings into a signed line error and detect loss/junctions without mistaking a barcode for the main line. The barcode sensor on GP16 records both black *and white* edge widths; encoder distance between transitions reduces speed dependence. The supplied examples show A and Z at two scales, but the final symbol set, direction, and command mapping remain to be confirmed. Decode a complete valid frame and emit a command once.
- **Motion:** Count both quadrature encoders with declared A/B polarity, measure counts per wheel revolution, wheel diameter, and wheel separation, then derive speed, distance, and encoder-based turns. Close each wheel's speed loop, limit outputs, and stop on command timeout, stall, or controller stop. Left/right line correction is an outer steering loop, not a substitute for motor PID.
- **Terrain:** The GY-511/LSM303DLHC has an accelerometer and magnetometer but **no gyroscope**. It can detect tilt/acceleration patterns and impacts; `yaw_rate_dps` in the scaffold cannot be a raw gyro value. Keep hump detection and physical-height validity separate. Agree on a defensible estimate using measured reference humps, travel/geometry, and error checks before reporting height in metres; do not treat uncorrected double integration as a height measurement. Keep the largest valid hump estimate for the run.
- **Obstacle:** Keep a fresh forward-range sample independent of side-scan samples. Use the older HC-SR04 sheet's more-than-60 ms trigger interval as a conservative starting point, then measure the installed HC-SR04P's timing; include servo settling and echo timeout in a nonblocking coarse/fine scan. Use car width, stopping distance, and safety margin to judge left/right clearance. If readings are missing or both sides are blocked, stop or take a bounded recovery action. Reacquire the original line before returning control to line following. Sonar can approximate a profile, not prove exact object shape.

### Mission arbitration

The target state machine is **idle/ready → follow line → execute navigation or scan/bypass → reacquire line → follow line**, with stopped/fault states and a finish state only after the course end rule is defined. Hump monitoring and telemetry run alongside motion. The controller gives physical stop/fault conditions first priority, then stale/invalid sensor or motion feedback, then obstacle handling, then barcode/junction actions, then ordinary line steering. Each maneuver needs entry criteria, completion evidence, timeout, and a safe exit. No network message may override a safety stop. The review permits validated status requests only and rejects drive/start/stop. The current bench additionally permits stopped black/white calibration as a user-requested implementation extension. The `s`/`x` USB commands are bench-only scaffold controls.

Use a monotonic timestamp at acquisition, an explicit validity flag, and separate status for optional profile/height values. Distances and heights are metres, speed metres/second, angles degrees, and line error is negative left/positive right. Treat navigation events as one-shot. Updates should finish promptly: sonar waits, servo settling, and WiFi reconnection must not block motor control. The target uses deadline scheduling: Safety 10 ms, Motion/Mission/line/Terrain 20 ms, required-sample age initially 250 ms, and commanded brake within 50 ms. These are design targets requiring timing measurements. The bench samples ADC every 20 ms and services networking in a loop sleeping 1 ms; it does not establish RTOS deadlines. `code/include/robot/config.h` contains demonstration values, not calibrated limits.

## Telemetry and verification boundaries

The intended MQTT publication covers mission state/reason, measured wheel speed/counts/distance, line/barcode status, hump events and highest valid height, forward and side obstacle data, plus validity/age and heartbeat/connection health. The review requires healthy local autonomy to continue through WiFi/MQTT loss. The bench uses `robot/car01/{telemetry,heartbeat,command,status}`, 500 ms telemetry, 1 s heartbeat, and a session/ID/age-validated command envelope. Credentials are in the ignored local header. These configured implementation details are not all fixed by the review. USB logs can support early bench tests but do not satisfy MQTT. Keep credentials out of source control and prevent stale or replayed subscribed messages from taking effect.

Before calling this architecture operational, show repeatable subsystem measurements, a physically verified stop, bounded turns, known-barcode trials, known-height hump trials, obstacle bypass with line reacquisition, and a full mission with telemetry. Log firmware revision, setup, expected and actual results, units, and evidence in [`code/docs/test-log.csv`](test-log.csv). The review defines planned T01–T16 trials and candidate timing/electrical targets, while course-level accuracy/pass thresholds still require agreement. No planned trial is a passed result.
