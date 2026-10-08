# Line following architecture

Owner: Nicholas (line sensing); Mahesha (motion/encoder control). Parent: [system architecture](../ARCHITECTURE.md). Wiring: [hardware](../HARDWARE.md). Design authority: [CSC01 Design Review](<../../../CSC01 Design Review.docx>), FR1/FR9 and Sections 3–6.

## Integrated target design

Line/Barcode samples both calibrated analogue IR channels every nominal 20 ms and publishes timestamped signed line error/junction/loss in `M_line`. Mission selects the behavior and sends an ID-bound leased target through `M_motion_cmd`; Motion alone drives GP8–GP11, uses encoder PID and publishes measured/stationary/completion feedback. Steering is an outer correction, not measured wheel-speed control.

Calibrate mounted black/white ranges and polarity, wheel geometry, counts/revolution, speed limits and stopping response. On recoverable loss, Mission performs bounded original-line recovery with valid forward clearance. Stable reacquisition resumes following; exhausted recovery stops. Stop/fault and stale required observations override steering. The target uses two line sensors; the current IR tracking bench implements calibrated two-sensor steering without the full-mission Motion/RTOS/encoder layer.

Tests: review T02/T03/T05/T11/T14/T15. Repeat straight/curve/T-junction trials, light/speed changes, encoder turn/stop accuracy and controlled loss/recovery. Numeric accuracy/lease/geometry limits require calibration; no completed full-mission trials are claimed.

## Current two-sensor black/white-calibrated bench

Active target: **`ir_tracking`**, requested on 6 October 2026. Left AOUT is Grove 5 / **GP26 / ADC0**; right AOUT is Grove 6 / **GP27 / ADC1**. Preserve the submitted Word map separately. Use 3V3/GND, leave both DOUT conductors and Grove 6's shared GP26 contact disconnected. Motor M1 left uses GP8/9; M2 right GP10/11. Motor lead/terminal and power guidance below also applies to this target. Servo GP12 is LOW; IMU, sonar, barcode and encoders remain unused.

### Calibrate both sensors on black and white

With motors stopped, capture **both black**, then **both white** from the dashboard or USB `b`/`w`. Keep each pose fixed for at least 500 ms and wait for acknowledgement. Use patches wide enough for both sensors to see that surface fully, at the same mounted height. Capture no edge or aligned reference.

Each channel stores its own raw black/white values. Both capture flags are required and each contrast must be at least 300 ADC counts. Each sensor can have a different ADC range or polarity. Calibration becomes ready even when current readings are fully white or black; starting still requires a visible line. If not ready, check both reference sets and contrast, then recapture with motors stopped. Calibration lives only in RAM and is lost on reboot.

A capture gathers at least 25 fresh paired samples over at least 500 ms. Overlapping captures, captures while driving or stop held are rejected; stop cancels an active capture, and a sample gap over 60 ms rejects it. Acknowledgement contains the request ID and result. Both references are committed together at completion.

### Error and PID

The source is split by purpose. **Nicholas only needs to own `ir_tracking.c`, `ir_sensor.c` and their headers for IR tracking.**

| File | Purpose | Ownership |
| --- | --- | --- |
| `src/ir_tracking.c` / `include/robot/ir_tracking.h` | Paired calibration, normalized readings, line classification, one PID and steering output | Nicholas: IR tracking |
| `src/ir_sensor.c` / `include/robot/ir_sensor.h` | GP26/GP27 ADC initialization, channel switching and averaging | Nicholas: IR acquisition |
| `src/motor_output.c` / `include/robot/motor_output.h` | Physical M1/M2 PWM, braking and motor direction | Motion teammate |
| `src/robot_control.c` / `include/robot/robot_control.h` | Start/stop state, freshness deadlines, loss recovery and conversion from steering to wheel duties | Application integration |
| `src/robot_app.c` | Firmware entry point and cooperative sensor/network scheduling | Application integration |
| `src/robot_telemetry.c` / `include/robot/robot_telemetry.h` | Schema 5 MQTT JSON snapshots; mode `ir_tracking` | Communications integration |

`ir_tracking.c` includes no Pico SDK, motor or MQTT headers and builds independently on a host. The old single-sensor follower has been removed. The mission scaffold and optional stationary sensor bench remain separate applications.

Every 20 ms, read left then right, discarding the first conversion after each ADC channel switch and averaging eight subsequent conversions. For each sensor:

```c
black_fraction = clamp((raw - white) / (black_reference - white), 0, 1);
```

This supports either ADC polarity independently. Form the signed error:

```c
error = right_fraction - left_fraction;
correction = Kp * error + Ki * integral + Kd * filtered_derivative;
left_pwm = clamp(base_pwm + correction, 0, max_pwm);
right_pwm = clamp(base_pwm - correction, 0, max_pwm);
```

Equal normalized black fractions give zero steering error, even with unequal raw ADC readings. Exact intermediate edge readings are not required for calibration. More black on the left makes error negative: left motor slower, right motor faster, steering left. More black on the right does the opposite. Confirm physical motor polarity with wheels raised before relying on the sign.

Starting settings: base 10%, per-wheel cap 35%, correction cap ±10%; `Kp=.10`, `Ki=0`, `Kd=.002`. Elapsed time uses seconds. Derivative filtering is `filtered += .25 * (new - filtered)`. Start with PD: tune P slowly, add D to damp wobble, consider I only for persistent bias. Integral is bounded to ±1 error-second with conditional anti-windup; `Ki=0` disables accumulation. Capture, stop and restart clear history, and the first derivative is suppressed. These are tuning defaults, not measured gains or wheel speeds.

### Reusable IR and PID functions

Two sensor readings produce **one combined error and one PID correction**. The public functions in `include/robot/ir_tracking.h` separate that calculation from the bench's motor-duty mixing:

1. `ir_tracking_reading(raw, &calibration)` validates references and returns normalized left/right fractions plus `right_fraction - left_fraction`.
2. `ir_tracking_classify(reading)` reports unknown, visible, lost or broad black. This is instantaneous classification; the application's loss hysteresis and grace remain in `robot_control_sample`.
3. `ir_tracking_pid_init(&pid)` sets the existing default gains once. `ir_tracking_pid_update(&pid, reading.error, dt_seconds)` updates one PID and stores the bounded result in `pid.correction`. It accepts no motor output or follower object.
4. `ir_tracking_pid_reset(&pid)` clears history while retaining gains. Call it whenever ordinary tracking is interrupted, such as stop, calibration, loss or ambiguous broad black.

For the motor-control teammate, obtain the module's typed output:

```c
/* After acquisition, calibration and an ordinary visible-line PID update. */
IrTrackingOutput output = ir_tracking_output(&tracker, now_ms);
/* Hand over output: valid, line, error, steering, sampled_ms. */
```

`output.steering` is in **[-1, 1]**, negative left and positive right. It equals `pid.correction / IR_MAX_CORRECTION`. Its `valid` flag is false for stale, invalid, uncalibrated, lost, broad-black, capture-in-progress or not-yet-processed samples. `sampled_ms` is the original acquisition timestamp; fresh line condition remains available even when steering is invalid. The consumer chooses its base speed, steering scale and wheel bounds and retains start/stop authority. Conceptually, `left_target = base + scale * steering` and `right_target = base - scale * steering`, then clamp each target in the motor module's units. This is a steering request, not measured speed or two separate sensor PIDs.

Call `ir_tracking_sample(&tracker, raw, now_ms)` with each paired ADC reading. It updates acquisition/calibration/classification only. For a fresh visible observation in the enabled tracking state, call `ir_tracking_update_pid(&tracker, dt_seconds)`; then read `ir_tracking_output`. Reset PID history when ordinary tracking is interrupted. The current `robot_control.c` performs these checks and consumes the same steering output to preserve the bench's existing 10% base and ±10% correction. Loss recovery and broad-black slow driving are separate application policies.

The low-level `ir_tracking_pid_update` remains available for direct calculations. Nonfinite/out-of-range errors, nonpositive/nonfinite elapsed time and nonfinite gains return false and clear history. Core calibration accepts paired black/white requests; `robot_control_capture` is the stationary/held-stop guard and communication-enum adapter. The IR module does not grant permission to move or enforce application start/stop policy.

### Local controls and ambiguous readings

Boot outputs are LOW/LOW. GP20 requires release plus fresh debounced press; held at boot cannot start. GP21/USB `x` stop and latch Stopped. One subsequent press resets to Ready, a separate press starts. Start requires fresh valid samples, valid calibration and a visible line. A control-update gap over 60 ms stops before accepting a new sample; the 250 ms watchdog returns a hung loop to stopped boot. Network loss alone does not stop healthy tracking.

Both fractions below 0.15 enter line-loss grace. Hold the last nonzero error direction at 5% base duty and ±5% correction for at most 100 ms; either fraction reaching 0.25 before expiry resumes with reset PID history. Otherwise latch Stopped. If no direction has been seen, grace uses equal 5% duties. Both above 0.85 are ambiguous broad black: straight at 5%, PID suspended. This does not classify a junction or choose a route. Ordinary PID cannot implement barcode-commanded turns or guarantee sharp corner recovery with two sensors.

### Build and verification

```sh
cmake -S . -B build-ir-tracking -DPICO_BOARD=pico_w -DPICO_SDK_PATH=/path/to/pico-sdk -DROBOT_BUILD_SENSOR_BENCH=OFF
cmake --build build-ir-tracking --target ir_tracking -j 6
sh scripts/check-ir-tracking.sh
sh scripts/check-sensor-bench.sh
node tests/test_dashboard_ui.js
python3 tests/test_mqtt_dashboard.py
```

Flash **`build-ir-tracking/ir_tracking.uf2`** using BOOTSEL/RPI-RP2. The target is named for its purpose; the retired follower is no longer built. Host tests cover independent sensor ranges/polarity and black/white-only readiness, paired capture/acknowledgement and rejections, PD filtering, bounded PWM/I anti-windup, loss hysteresis/deadlines, stop/reset/held-start and JSON bounds. The actual dashboard renderer checks tracking and stationary modes, and the browser layout was inspected using synthetic readings. Local broker-to-dashboard, white-capture command and synthetic acknowledgement checks passed on an isolated topic. Physical motor direction, stop response, timing under actual network load and repeatable line tracking require hardware trials; record them in `docs/test-log.csv`.

## Wiring (power off first)

Use two suitable wires per motor, secured in the Robo Pico screw terminals. Reusing a wire does not reuse its old Grove port. Do not attach motor power leads to Grove 2/3, 3V3 or GPIO headers.

| Original motor lead | Robo Pico terminal | Controller pin already wired inside carrier |
| --- | --- | --- |
| **Left black, motor +** | **M1A** | GP8 |
| **Left yellow, motor −** | **M1B** | GP9 |
| **Right black, motor +** | **M2A** | GP10 |
| **Right yellow, motor −** | **M2B** | GP11 |

These colors are the motor leads in the project wiring references. Reused extension-wire colors were not supplied: label each wire at both ends and follow the motor lead/terminal, rather than guessing by extension color. Use wires rated for the actual motor current and secure terminal contact. With wheels raised, verify physical forward direction; if a wheel runs backwards, swap that motor's black/yellow terminal connections with power off (or change its `MOTOR_*_FORWARD_A` setting in `include/robot/motor_output.h` and rebuild).

Both IR channels use the current wiring table above. Servo GP12 stays LOW and no servo PWM is generated. Encoders, IMU and sonar remain unused by this tracking bench.

## Reflectance sensor technical reference

### Supplied device references describe different products

- [Infrared-Reflective-Sensor-UserManual.pdf](../../../document/Infrared-Reflective-Sensor-UserManual.pdf) describes a **Waveshare ST188/LM393 module** with `DOUT`, `AOUT`, `GND`, and `VCC` (3.0–5.3 V). `AOUT` is a reflectance-related analogue signal and `DOUT` is thresholded by the onboard comparator. Its example wiring uses 3.3 V for an STM32 and 5 V for an Arduino; these are examples, not the team's pin choices. [pp. 1–2]
- [tcrt5000.pdf](../../../document/tcrt5000.pdf) describes the **bare Vishay TCRT5000/TCRT5000L** emitter/phototransistor, not a powered four-pin module. It needs an LED current-limiting circuit and a detector load/conditioning circuit. Its peak operating distance is **2.5 mm**; its relative response remains above 20% over roughly **0.2–15 mm** in the stated test. The emitter wavelength is about 950 nm and the package has a visible-light filter. Actual black/white contrast depends strongly on mounting height and surface. [pp. 1–4]
- [LM393-D.PDF](../../../document/LM393-D.PDF) describes the **comparator IC**, not a whole sensor module. It compares analogue voltages and its output needs a suitable pull-up/board circuit; a module powered at 5 V may present a 5 V digital pull-up even if the bare comparator can run over a broad supply range. Its IC supply range of 2–36 V is **not** permission to feed 5 V into Pico GPIO. Check the actual module's output high level and pull-up rail. [pp. 1–3]

The documents do not establish whether the team's three IR modules are ST188 modules, TCRT5000 modules, another model, or a mix. Do not assume a TCRT5000 chip datasheet proves the pinout of a four-pin board.

**Waveshare module information transcribed from its two-page manual:** the reflective element is listed as ST188, comparator as LM393, allowed module supply 3.0–5.3 V, board size 30.2 × 11.9 mm, and fixing-hole size 2.0 mm. Its connector order is **1 DOUT, 2 AOUT, 3 GND, 4 VCC**. The manual's STM32 example uses DOUT GPIOA.4, AOUT GPIOA.6 and 3.3 V supply; its Arduino example uses D2, A0 and 5 V. The example serial setting is 115200 baud, 8 data bits, no parity, 1 stop bit; this is the demonstration program, **not** a serial interface on the IR module. Its onboard indicator lights when close to a barrier in the manual's example. The manual does not state which DOUT logic level means black, the exact comparator threshold, or the best mounting height for this course. [Infrared manual pp. 1–2]

**Bare TCRT5000 limits and optics:** it combines an IR LED and phototransistor in a 10.2 × 5.8 × 7 mm leaded package; TCRT5000L has longer leads. The peak relative response occurs at about **2.5 mm** from a reflective target, and the quoted >20% relative-current distance range is **0.2–15 mm** under the datasheet's test conditions. The emitter has a visible-light blocking package, approximately 940–950 nm peak emission, about 1.25 V typical forward drop at a stated 60 mA test current, and **60 mA absolute maximum continuous forward current**; this maximum is not a recommended design current. The detector's stated collector-emitter maximum is 70 V and collector-current maximum 100 mA, but neither allows Pico GPIO to exceed 3.3 V. The sensor needs external LED current limiting and detector load/threshold circuitry; it is not interchangeable pin-for-pin with the Waveshare module. Reflectance changes with surface, height, temperature and LED current, so calibrate on the actual black line and white floor at the mounted height. [tcrt5000 pp. 1–4]

**LM393 comparator IC details:** two independent comparators are inside one 8-pin IC. On the bare IC, pins are `1=OUT A`, `2=IN− A`, `3=IN+ A`, `4=GND`, `5=IN+ B`, `6=IN− B`, `7=OUT B`, `8=VCC`. Its single-supply operating range is 2–36 V, but its input common-mode high end is about `VCC − 1.5 V` at 25 °C (or `VCC − 2.0 V` over the listed temperature range). At 5 V and 25 °C the LM393 family's typical supply current is 0.4 mA; the comparator can sink output current and needs an output pull-up to produce a high level. Pull-up **voltage**, not the broad chip VCC range, determines what a Pico digital input sees. Noise near the threshold can make DOUT chatter; the datasheet discusses adding a small amount of positive feedback (hysteresis) and keeping output traces away from sensitive inputs. If the module already contains these components, do not duplicate them blindly. [LM393 pp. 1, 3, 5]

| Question for line tracking | What to observe on the assembled sensor |
| --- | --- |
| Black vs white | Log `AOUT` if connected and `DOUT` on both surfaces; record the actual polarity. |
| Height | Compare black/white separation at the real chassis clearance, not the bare TCRT5000 datasheet's ideal test distance. |
| Lighting and speed | Test under the demonstration lighting and at slow/normal travel speeds. |
| Comparator threshold | Adjust or record the module's trim setting if present; use hysteresis/debounce where transitions chatter. |
| Electrical levels | Power module from a verified rail and measure DOUT HIGH before connecting to 3.3 V Pico GPIO. |
