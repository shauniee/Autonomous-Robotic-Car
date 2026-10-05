# Line following architecture

Owner: Nicholas (line sensing); Mahesha (motion/encoder control). Parent: [system architecture](../ARCHITECTURE.md). Wiring: [hardware](../HARDWARE.md). Design authority: [CSC01 Design Review](<../../../CSC01 Design Review.docx>), FR1/FR9 and Sections 3–6.

## Integrated target design

Line/Barcode samples both calibrated analogue IR channels every nominal 20 ms and publishes timestamped signed line error/junction/loss in `M_line`. Mission selects the behavior and sends an ID-bound leased target through `M_motion_cmd`; Motion alone drives GP8–GP11, uses encoder PID and publishes measured/stationary/completion feedback. Steering is an outer correction, not measured wheel-speed control.

Calibrate mounted black/white ranges and polarity, wheel geometry, counts/revolution, speed limits and stopping response. On recoverable loss, Mission performs bounded original-line recovery with valid forward clearance. Stable reacquisition resumes following; exhausted recovery stops. Stop/fault and stale required observations override steering. The target uses two line sensors; the current bench below uses one edge sensor.

Tests: review T02/T03/T05/T11/T14/T15. Repeat straight/curve/T-junction trials, light/speed changes, encoder turn/stop accuracy and controlled loss/recovery. Numeric accuracy/lease/geometry limits require calibration; no completed full-mission trials are claimed.

## Current right IR bench implementation

Current active firmware: **`right_ir_follow_mqtt`** on branch `feat/mqtt-sensor-bench`. This replaces the previous stationary sensor bench for Nicholas's current setup: IMU and ultrasonic disconnected to reuse their wires, servo disabled, right IR plus two drive motors. The Word document is untouched.

## Wiring (power off first)

Use two suitable wires per motor, secured in the Robo Pico screw terminals. Reusing a wire does not reuse its old Grove port. Do not attach motor power leads to Grove 2/3, 3V3 or GPIO headers.

| Original motor lead | Robo Pico terminal | Controller pin already wired inside carrier |
| --- | --- | --- |
| **Left black, motor +** | **M1A** | GP8 |
| **Left yellow, motor −** | **M1B** | GP9 |
| **Right black, motor +** | **M2A** | GP10 |
| **Right yellow, motor −** | **M2B** | GP11 |

These colors are the motor leads in the project wiring references. Reused extension-wire colors were not supplied: label each wire at both ends and follow the motor lead/terminal, rather than guessing by extension color. Use wires rated for the actual motor current and secure terminal contact. With wheels raised, verify physical forward direction; if a wheel runs backwards, swap that motor's black/yellow terminal connections with power off (or change its `FOLLOW_*_FORWARD_A` setting and rebuild).

Right IR now uses **Grove 6**: **AOUT → GP27/ADC1, VCC → 3V3, GND → GND**. DOUT stays disconnected. Left IR moves to **Grove 5 / GP26 / ADC0** and is unused by this one-sensor firmware. Leave Grove 6’s shared GP26 contact disconnected. IR wire colors are not documented; follow the module's AOUT/VCC/GND labels. The four encoder wires (blue GND, white supply, red A, green B) are **not motor-power leads**; encoders are not used by this firmware, so leave unused ends insulated.

Servo: disconnect its plug; the new firmware holds **GP12 LOW**, generates no servo PWM and accepts no servo controls. IMU/sonar ports GP2–GP5 are unused. Power-bank red/black continue to VIN +/− per the existing design. Motor leads go to M1/M2 only; no extra wires from motor terminals to Pico GPIO are needed.

## Flash and operate

Flash **`build-right-ir-follow/right_ir_follow_mqtt.uf2`**, not the earlier sensor-bench UF2. Hold BOOTSEL while connecting USB, then copy to RPI-RP2. Boot keeps both motors braked (LOW/LOW) until local start.

1. Open the MQTT dashboard at **http://127.0.0.1:8765/**, with the wheels raised and the motors stopped. Hold the right IR over black and click **Capture black**; hold it over white and click **Capture white**. Wait for the robot’s capture acknowledgement after each click. Each captures a fresh averaged ADC value. At least 300 counts contrast is required. Calibration lives in RAM and must be repeated after reboot.
2. Place the right IR near the **right edge of the black line**. It may read fully black or fully white; no intermediate ADC range is required. Press/release **GP20** to start after calibration. Initially verify both wheels move forward with wheels raised, then perform a low-speed track trial.
3. **GP21** or USB **`x`** brakes both motors and latches Stopped. A fresh GP20 press resets to Ready only; release and press again to start. Held start at boot cannot start it; held stop inhibits motion.
4. More black under the right sensor makes the left wheel faster, steering right; more white makes the right wheel faster. The controller targets the black/white boundary. All-black/all-white readings continue steering; they do not trigger a timeout because this sensor may switch directly between the two extremes. Invalid calibration still stops the motors.

With only one IR, this follows a chosen **edge**, not the centre using two sensors. It cannot tell which side it is on after losing the line. There is no obstacle avoidance, barcode decoding, IMU processing, encoder speed control or physical speed measurement in this mode. Starting duty is 10%, capped at 35%; gain and mounting need real-car tuning. Without an edge-loss timeout, it keeps turning on white even if the track is absent; use GP21 to stop.

The [dashboard](http://127.0.0.1:8765/) displays raw IR, calibration, edge error, state/reason and applied motor PWM when this firmware publishes. MQTT settings are retained in the ignored local header; the broker is on this Mac at 192.168.0.8:1883. MQTT supports black/white calibration while stopped, in addition to status requests. It never starts/drives/stops the car. Calibration requests use a fresh session/timestamp and one-shot ID; stale, duplicate and busy requests are rejected. Network loss does not block local line following; a 250 ms watchdog returns a hung loop to braked boot.

## Rebuild and checks

```sh
cmake -S . -B build-right-ir-follow -DPICO_BOARD=pico_w -DPICO_SDK_PATH=/path/to/pico-sdk -DROBOT_BUILD_SENSOR_BENCH=OFF
cmake --build build-right-ir-follow --target right_ir_follow_mqtt -j 6
cc -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude src/right_ir_follow.c tests/test_right_ir_follow.c -lm -o build-host/test_right_ir_follow
build-host/test_right_ir_follow
```

The local real SDK firmware build and pure steering checks passed. Actual Pico MQTT telemetry and right-IR readings were subsequently observed. Motor polarity, braking response and repeatable physical line following still require measured trials. The donor wires' actual colors/rating also remain unverified.

## Speed and steering tuning

Edit `include/robot/right_ir_follow.h` from the repository root. Current values: `FOLLOW_BASE_DUTY .10f`, `FOLLOW_MAX_DUTY .35f`, `FOLLOW_KP .10f`. Higher base increases requested power; the maximum caps each wheel; higher gain makes corrections sharper. Rebuild/flash after editing. Black/white calibration is separate and available through MQTT.

The implementation averages eight ADC readings every 20 ms, normalizes `(raw-white)/(black-white)`, clamps black fraction to 0–1 and uses error `2*fraction-1`. Duties are clamped `base ± gain*error`. With current values, extreme black/white requests approximately 20% on one wheel and 0% on the other. Servo/IMU/sonar/encoders are unused in this mode.

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
