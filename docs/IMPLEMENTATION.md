# Current code structure and evidence

The [architecture](ARCHITECTURE.md) is the proposed integrated FreeRTOS design. This file describes the implemented code, which does not yet implement that task graph.

## Firmware targets

| Target | Entry point | Status |
| --- | --- | --- |
| `right_ir_follow_mqtt` | `src/main_right_ir_follow.c` | Active cooperative bench: actual GP27 ADC, M1/M2 PWM, local buttons, calibration/status MQTT |
| `sensor_bench_mqtt` | `src/main_sensor_bench.c` | Optional stationary IR/GY-511/servo-sonar acquisition; disabled by default |
| `robot_car` | `src/main_pico.c` | Full-mission controller scaffold with invalid/no-op sensor/motor drivers and WiFi backend when enabled |

Each target has its own `main`; only the flashed target runs. Do not flash the scaffold expecting the bench motor follower.

## Active flow

1. `main_right_ir_follow.c` initializes braked outputs, GP27 ADC and GP20/21, networking and a 250 ms watchdog.
2. The stopped dashboard captures black/white via MQTT. References live in RAM; reboot requires recapture. Minimum contrast is 300 ADC counts.
3. Fresh GP20 starts after valid calibration, even on fully black/white. GP21 brakes; a reset press does not start movement.
4. Every 20 ms, eight ADC samples are averaged. `right_ir_follow.c` normalizes black fraction, calculates signed error, then clamps `base ± gain × error` to the duty cap. Extreme readings keep steering; there is no edge-loss timeout.
5. `comms_picow.c` services WiFi/lwIP without synchronous connection waits, retries with backoff and publishes telemetry every 500 ms plus heartbeat every second. Network loss does not stop healthy local tracking.
6. `comms_protocol.c` validates session, increasing ID, uptime age and message shape. Only status and stopped black/white calibration are accepted; remote start/stop/drive are rejected.
7. On the Mac, Mosquitto is the broker. `scripts/mqtt_dashboard.py` subscribes/publishes using Mosquitto CLI and serves `dashboard/index.html` plus its HTTP API. The browser polls the API and shows freshness, ADC, calibration and commanded PWM.

`include/robot/right_ir_follow.h` holds GPIO and tuning. Current settings: base 0.10, maximum 0.35, gain 0.10. PWM is commanded power, not measured speed. The active target also compiles `controller.c` because the shared protocol retains the legacy mission formatter; the bench does not call that controller to steer.

## Remaining structure

`include/robot/` contains shared types/contracts; `src/subsystems/` contains mission modules and networking. `tests/` provides pure control/protocol checks and fake-SDK driver/network tests. `config/network_config.h` is ignored private configuration; `wifi/include/lwipopts.h` configures lwIP. `scripts/` contains build/check and monitoring tools. Project and technical Markdown lives in this directory.

The teammate `wifi_telemetry` branch supplied WiFi/MQTT connection, retries, heartbeat and legacy start/stop tools. This branch is rebased onto it and changes the subscription policy to match the design’s local motion controls, with user-requested stopped calibration added. [WiFi guide](WIFI_TELEMETRY.md) documents the reconciled current transport contract.

## Verification boundary

Real SDK firmware builds, host steering/protocol/controller checks and fake-SDK driver/network checks passed after integration. Actual Pico MQTT telemetry and right-IR raw readings were observed during the current bench session. Physical speed, polarity/braking response, repeatable line tracking, encoder PID, RTOS scheduling, barcode, hump height, obstacle bypass and full mission acceptance are not established by those checks. The review’s T01–T16 are planned trials, not passed results.
