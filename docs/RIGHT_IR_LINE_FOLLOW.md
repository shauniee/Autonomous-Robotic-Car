# Right IR line following — servo disabled

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
2. Position the right IR over the **right edge of the black line**, around halfway between its black/white values. Press/release **GP20** to start. Initially verify both wheels move forward with wheels raised, then perform a low-speed track trial.
3. **GP21** or USB **`x`** brakes both motors and latches Stopped. A fresh GP20 press resets to Ready only; release and press again to start. Held start at boot cannot start it; held stop inhibits motion.
4. More black under the right sensor makes the left wheel faster, steering right; more white makes the right wheel faster. The controller targets the black/white boundary. If it stays all-black/all-white for 1500 ms, it stops with `edge_lost_timeout`.

With only one IR, this follows a chosen **edge**, not the centre using two sensors. It cannot tell which side it is on after losing the line. There is no obstacle avoidance, barcode decoding, IMU processing, encoder speed control or physical speed measurement in this mode. Starting duty is 22%, capped at 35%; gain, mounting and edge-loss timeout need real-car tuning.

The [dashboard](http://127.0.0.1:8765/) displays raw IR, calibration, edge error, state/reason and applied motor PWM when this firmware publishes. MQTT settings are retained in the ignored local header; the broker is on this Mac at 192.168.0.8:1883. MQTT supports black/white calibration while stopped, in addition to status requests. It never starts/drives/stops the car. Calibration requests use a fresh session/timestamp and one-shot ID; stale, duplicate and busy requests are rejected. Network loss does not block local line following; a 250 ms watchdog returns a hung loop to braked boot.

## Rebuild and checks

```sh
cmake -S . -B build-right-ir-follow -DPICO_BOARD=pico_w -DPICO_SDK_PATH=/path/to/pico-sdk -DROBOT_BUILD_SENSOR_BENCH=OFF
cmake --build build-right-ir-follow --target right_ir_follow_mqtt -j 6
cc -std=c11 -Wall -Wextra -Werror -pedantic -Iinclude src/right_ir_follow.c tests/test_right_ir_follow.c -lm -o build-host/test_right_ir_follow
build-host/test_right_ir_follow
```

The local real SDK firmware build and pure steering checks passed. Motor polarity, actual braking, physical line following and Pico-to-MQTT behavior are unverified until the robot is connected and tested. The donor wires' actual colors/rating also remain unverified.
