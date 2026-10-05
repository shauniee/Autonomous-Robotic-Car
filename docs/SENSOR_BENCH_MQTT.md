# MQTT physical sensor bench — motors disconnected

Branch: **`feat/mqtt-sensor-bench`**. Firmware target: **`sensor_bench_mqtt`**.

This optional legacy target uses the review’s original right IR GP26, not the active follower’s GP27 swap. Its servo/IMU/sonar setup is not the current disconnected setup.

This firmware reads the actual right IR ADC, GY-511 acceleration/magnetic field and servo-mounted HC-SR04P, then publishes timestamped measurements over WiFi/MQTT. Both drive motors are reported disconnected, and GP8–GP11 stay LOW; this target does not link a motor motion driver or configure motor PWM. It monitors sensors on a stationary robot, so line-following movement is outside this mode.

`CSC01 Design Review.docx` was read for the pin assignments and **not modified**. The project references remain the target mission design; this bench is an intermediate implementation, not a completed autonomous mission. The teammate's `wifi_telemetry` backend (commit `828f6f9`) is reused with a status-only subscription; remote start/stop/drive are rejected. Network loss leaves local sensor acquisition running.

## Connections from the Word design

| Component | Signals | Supply / notes |
| --- | --- | --- |
| Right IR | **GP26 / ADC0 / AOUT** | Grove 5 3V3/GND; DOUT unused |
| GY-511 / LSM303DLHC | **GP4 SDA, GP5 SCL, I²C0** | Grove 3 3V3 to Vin and common GND; leave separate `3.3`, INT and DRDY unused |
| HC-SR04P | **GP2 Echo, GP3 Trig** | Grove 2 3V3/GND; installed 3.3 V-capable variant required |
| SG90 servo | **GP12 signal**, Servo 1 | Orange to S, red to +, brown to −; carrier servo rail, not 3V3 |
| Local scan toggle | **GP20** | Onboard button; fresh press after release |
| Local servo off | **GP21** | Held press overrides scan commands |
| Drive motors | **GP8–GP11** | Held LOW; motors remain disconnected |

Check actual labels/pin order, common ground, Echo/I²C/IR highs within 3.3 V, and servo rail compatibility before powering the sensor setup. The SG90 needs its rated supply from the carrier; the Pico's USB logic alone does not establish the actual servo rail voltage. These are the project's existing electrical checks, not claims that wiring has been verified.

## What the readings mean

- Right IR: averaged 12-bit ADC every 20 ms, nominal voltage assuming 3.3 V reference. Raw readings work without calibration. USB `b` captures black and `w` white from the latest sample. With at least 300 counts contrast, normalized black fraction is reported. Both sensor polarities are supported. Calibration lives in RAM.
- Accelerometer: address 0x19, 50 Hz, ±2g, high-resolution 12-bit mode with BDU. Read XYZ coherently using auto-increment; output raw signed counts and g.
- Magnetometer: address 0x1E, 30 Hz continuous, ±1.3 gauss. Decode device order X/Z/Y into XYZ and use separate XY/Z gains. Overflow readings are invalid. No calibrated magnetic heading is claimed.
- Pitch/roll: derived from gravity in the sensor's own axes, valid only while acceleration magnitude is near 1g. Mounting/offset calibration and dynamic-motion validation remain pending. The GY-511 has **no gyroscope**, so yaw rate and hump height are `null`.
- Ultrasonic: 10 µs trigger, minimum 65 ms trigger interval, interrupt-captured Echo edges and 30 ms timeout. Approximate conversion follows the supplied pulse-ranging principle; missing/out-of-range echoes are invalid. Scan readings never refresh the separate forward reading.
- Servo: 50 Hz PWM, nominal 1000–2000 µs endpoints, 1500 µs centre. Report **commanded** angle/pulse; the SG90 supplies no actual angle feedback. Calibrate the installed servo's centre/range/settling. Bench sweep is deliberately limited to 60°/90°/120°, with centre revisited between side samples and a 300 ms settling delay.

I²C transactions have finite 2 ms timeouts. A missing device is marked invalid and configuration retried every 2 s. Sample ages/validity are included; old IMU data expires after 250 ms and range data after 1500 ms. The 1 s watchdog restarts a stuck loop. This cooperative bench is not the proposed FreeRTOS mission scheduler and makes no hardware timing guarantee.

## Flash and view

The current local configuration contains the supplied WiFi credentials and the broker LAN IP **192.168.0.8**, port **1883**, topic root **robot/car01**. Credentials are only in `config/network_config.h`, which is Git-ignored and not committed. The broker runs on this Mac; both the Mac and Pico must reach each other on the WiFi LAN. If the Mac's DHCP address changes, update the broker IP and rebuild.

Build from `code/` with the installed SDK/toolchain:

```sh
cmake -S . -B build-sensor-bench -DPICO_BOARD=pico_w -DPICO_SDK_PATH=/path/to/pico-sdk -DCMAKE_BUILD_TYPE=Release
cmake --build build-sensor-bench --target sensor_bench_mqtt -j 6
```

The SDK used for the verified local build is `/Users/nicholasbay/Documents/school/INF2004/pico-sdk`.

1. Hold **BOOTSEL** while connecting the Pico by USB.
2. Copy **`build-sensor-bench/sensor_bench_mqtt.uf2`** to **RPI-RP2**. Do not use the earlier single-IR driving firmware or the `robot_car` mission scaffold.
3. Open **http://127.0.0.1:8765/** on this Mac. Once the Pico joins WiFi/MQTT, live readings should appear automatically; no start button is needed for sensing.
4. Move a black/white card under the right IR, gently tilt the board, and move a target in front of the sonar to check actual responses. Record measurements; a compiled driver is not proof of sensor behavior.
5. Press **GP20** to start side scanning; press again to return to centre. **GP21** disables servo PWM. A fresh GP20 press after servo-off restores centre; another enables scanning. The motors remain disabled throughout.

USB serial also prints the same JSON every 500 ms. USB commands: `a` scan, `c` centre, `x` servo off, `b` capture black, `w` capture white. Servo-off stops pulses; it does not provide a mechanical brake or measured position.

## Restart the Mac tools

Mosquitto was installed through Homebrew and started locally; no login-start service was enabled. From `code/`, if the broker/dashboard are no longer running:

```sh
/opt/homebrew/sbin/mosquitto -c config/mosquitto-lab.conf -d
python3 scripts/mqtt_dashboard.py
```

The dashboard has no pip dependencies; it uses Mosquitto's subscriber CLI. It listens only on localhost. The broker configuration permits anonymous plain MQTT on the lab LAN. Raw messages can also be viewed with:

```sh
/opt/homebrew/bin/mosquitto_sub -h 127.0.0.1 -t 'robot/car01/#' -v
```

The optional terminal viewer `scripts/mqtt_monitor.py` uses the Paho dependency in `scripts/requirements-mqtt.txt`. The browser dashboard is the ready-to-use viewer.

## Verified on 5 October 2026

- Real Pico W/WH SDK firmware build completed with `-Wall -Wextra -Werror` and produced the UF2.
- `sh scripts/check-sensor-bench.sh` passed: sensor scaling/order, invalid/saturated readings, telemetry bounds/JSON, status-only requests, real driver logic against a fake SDK (pin allocation, motor inhibition, IMU configuration/recovery, echo IRQ/timeout, servo settling and separate forward age), controller regression and network failure/reconnect cases.
- Actual local broker publish/subscribe passed on localhost and **192.168.0.8**.
- `python3 tests/test_mqtt_dashboard.py` passed against the real broker on an isolated synthetic test topic. The real robot dashboard was left without synthetic readings and shows **waiting for robot**.
- No Pico USB serial/BOOTSEL device was present. **Flashing, Pico-to-WiFi/MQTT connection, physical wiring, sensor response and actual servo motion are not yet verified.** The Word document's SHA-256 stayed `81f5f55d275c46e748d4e44530fec3c3b1efc908a3e8b1e10b43554590b5f4cf`.

Record physical trials separately in `docs/test-log.csv` after flashing.
