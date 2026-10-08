# Communications architecture

Owner: Shaun. Parent: [system architecture](../ARCHITECTURE.md).

Transport source: `src/subsystems/comms_picow.c`, originally supplied on the teammate’s `wifi_telemetry` branch. The current branch builds on that transport. Primary policy: [design review FR12 and Section 4.3](<../../../CSC01 Design Review.docx>); [implementation](../ARCHITECTURE.md) explains the active application.

## Connection and configuration

Pico WH onboard WiFi → access point → LAN Mosquitto broker on the Mac → dashboard subscriber. `config/network_config.h` contains private SSID/password, IPv4 broker, port and topic root; it is ignored by Git. `config/network_config.example.h` is the nonsecret template. Build with `PICO_BOARD=pico_w`; no external radio/MQTT library is needed beyond Pico SDK/lwIP and its initialized submodules.

The current lab endpoint is 192.168.0.22:1883 and topic root `robot/car01`; update the ignored header and rebuild if the Mac’s LAN address changes. `config/mosquitto-lab.conf` is the local anonymous lab-broker configuration. It is an implementation setup, not a confirmed final mission deployment/security decision.

```sh
mosquitto -c config/mosquitto-lab.conf -d
python3 scripts/mqtt_dashboard.py --port 8765
```

Open http://127.0.0.1:8765/ on the Mac. Broker and dashboard are separate processes. The Pico connects to the Mac’s LAN address, not localhost. The dashboard uses installed `mosquitto_sub`/`mosquitto_pub`; no Python MQTT package is required for this path. `scripts/mqtt_monitor.py` is an alternative monitor. Legacy `mqtt_client.py` start/stop actions are disabled for this branch; use physical GP20/21.

## Topics and freshness

| Topic suffix | Purpose | Delivery |
| --- | --- | --- |
| `/telemetry` | Current application state, readings and validity/age | QoS 0, not retained; nominal 500 ms |
| `/heartbeat` | Online/session/uptime and command/publish diagnostics | QoS 0, not retained; 1 s |
| `/status` | Online state and broker last will | QoS 1, retained |
| `/command` | Validated reporting/calibration request | Subscription QoS 1; requests sent without retain |

Do not mistake a retained online status for fresh telemetry. Dashboard freshness uses actual receive times. Motor duties are requested PWM, never encoder-measured speeds.

## Subscription and local motion

The review permits **status requests only**, rejects remote driving/start/stop and requires healthy local autonomy to continue through WiFi/MQTT loss. The active bench adds user-requested `calibrate_black`/`calibrate_white` while stopped. The reverted two-sensor bench no longer accepts `calibrate_aligned`; the protocol rejects it as an unsupported command. Firmware rejects calibration while moving. All accepted requests use exactly `session`, `id`, `issued_ms` and `command`; the current session/uptime come from a fresh heartbeat, ID increases and maximum accepted age is 5 s. Wrong sessions, stale/replayed messages, malformed fields and overlapping pending calibration are rejected.

Illustrative implemented envelope:

```json
{"session":"0123456789abcdef","id":1,"issued_ms":1000,"command":"status"}
```

Use actual fresh heartbeat values, not these constants. The review’s `{"request":"status"}` is an illustrative design example, not the current parser format. GP20/GP21 remain the motion controls.

The backend uses asynchronous connection attempts, bounded deadlines and exponential backoff up to 30 s. Offline telemetry is dropped instead of accumulated. It does not inject a stop on network loss or start motion on reconnect. Actual scheduling and recovery timing still require measured trials under load.

## Evidence

SDK builds and fake-SDK startup/reconnect/protocol checks passed; actual Pico MQTT/right-IR telemetry was observed in the bench session. Broker availability does not establish encoder measurement, complete mission operation or review T12/T13/T15 acceptance. Record physical trials in [test-log.csv](../test-log.csv).

## Integrated target contract and verification

Communications is proposed FreeRTOS priority 1. It consumes `M_telemetry` and queues validated status requests in `Q_status`; it has no actuator ownership. Bounded network work must yield below Safety/Motion/Mission. The implemented cooperative backend services the same transport from `comms_poll`, without RTOS task/mailbox wrappers yet.

Full mission reporting includes requested versus measured speeds/counts/distance, line/junction/barcode, terrain/maximum valid height, independent front/profile validity and acquisition ages, mission reason and connection health. IR tracking schema 5 / mode `ir_tracking` reports independent left/right raw ADC and black/white references, normalized black fractions, PID error/correction/integral/filtered derivative, line condition, state and applied PWM. Measured speeds are null. Capture status includes `request_id`, `result`, `busy`, `active_request_id`, sample count, reference flags and calibration readiness. Each paired capture averages at least 25 samples over at least 500 ms; moving, busy, held-stop or invalid/gapped capture is rejected or cancelled. The dashboard supports current IR tracking and the optional stationary sensor bench; the retired follower mode has been removed. Dashboard capture succeeds only after matching robot acknowledgement; stale state/heartbeat or moving motors inhibit requests. Calibration references live in RAM.

Review T12/T13/T14/T15 cover actual broker messages, prohibited/replayed commands, disconnect/reconnect under sensor load and bounded timing. Preserve freshness and command validation even when simplifying the code.
