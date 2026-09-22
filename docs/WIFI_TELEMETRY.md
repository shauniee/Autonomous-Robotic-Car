# Pico W WiFi and telemetry

Buddy 1 now has a Pico W C implementation. It joins WiFi asynchronously, connects
to an IPv4 MQTT broker, publishes measurements, receives validated start/stop
commands, sends heartbeats, and reconnects with backoff. The same main loop still
owns the controller and motion calls.

`MACHINE.MD` records ROBO PICO and MAKER PI PICO carrier boards. This implementation
targets the **Pico W module** discussed in the conversation; confirm that the
module mounted on your carrier is the wireless variant. No carrier GPIOs are used
for WiFi. Regular Pico builds can still select the USB-only backend.

## 1. Configure your network locally

If `config/network_config.h` does not exist, copy the example:

```powershell
Copy-Item config/network_config.example.h config/network_config.h
```

Edit that local file:

```c
#define ROBOT_WIFI_SSID "YOUR_2_4_GHZ_WIFI"
#define ROBOT_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define ROBOT_MQTT_BROKER_IP "192.168.1.20"
```

Use your laptop/broker's actual LAN IPv4 address, not `localhost` or the car's IP.
The car and laptop must be able to reach one another through the test network;
guest/client isolation can prevent this even on the same SSID. The initial backend
supports open WiFi or WPA2 personal authentication, not an enterprise login portal.

Optional settings are broker port/username/password, topic root and stop-on-loss
policy. Settings are C string literals: escape `"` and `\` inside credentials.
They are embedded in the firmware. The local header and build folders are ignored
by Git; do not commit credentials or share a configured UF2 publicly.

Empty SSID or broker IP disables networking with a USB message. Invalid numeric
IP/topic configuration also disables networking. Changing settings requires a rebuild.

## 2. Start a local broker

Install [Eclipse Mosquitto](https://mosquitto.org/download/) on the laptop. From the
repository root, start a foreground broker with the supplied lab configuration:

```powershell
& 'C:\Program Files\mosquitto\mosquitto.exe' -c config/mosquitto-lab.conf -v
```

This listens on TCP 1883 on all laptop interfaces. Permit it on the private test
network if Windows Firewall asks. If a Mosquitto service already uses port 1883,
use that broker's configuration instead or choose another port in both configs.

The example broker permits anonymous clients. This first implementation uses
plain MQTT without TLS and a numeric IPv4 address (no DNS): use it only on your
trusted bench network. For a password-protected broker, set matching username/
password values in the local header. TLS-only/cloud brokers are not supported yet.

## 3. Build and flash

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-picow.ps1
```

The script uses a configured SDK/toolchain, or the project-local dependencies in
`tmp/` if they are present. A fresh clone needs CMake, Ninja, an ARM GCC toolchain,
and a Pico SDK checkout with its CYW43, lwIP and TinyUSB submodules initialized.
Set `PICO_SDK_PATH` and, if needed, `PICO_TOOLCHAIN_PATH` for your installation.
See [official SDK setup](https://github.com/raspberrypi/pico-sdk).

Hold BOOTSEL while connecting your Pico W. Copy `build-picow/robot_car.uf2` to its
USB drive. Open USB serial after it reboots. Expected progress messages include:

```text
[net] Joining WiFi
[net] WiFi connected; IP ...
[net] Connecting to MQTT broker
[net] MQTT online; command subscription ready
```

Early boot messages can be missed if the serial monitor opens late. Periodic JSON
status continues over USB. An absent USB reader does not impose the default long
stdout wait; USB debug output may be dropped when the host cannot keep up.

## 4. Monitor and send commands

In a second terminal, install the laptop tool dependency:

```powershell
python -m pip install -r scripts/requirements-mqtt.txt
python scripts/mqtt_client.py monitor --broker 192.168.1.20
```

In another terminal:

```powershell
python scripts/mqtt_client.py stop --broker 192.168.1.20
python scripts/mqtt_client.py start --broker 192.168.1.20
```

Use your actual broker IP. Optional `--root` and `--port` must match the firmware.
For broker authentication, set `ROBOT_MQTT_USERNAME` and `ROBOT_MQTT_PASSWORD` in
the laptop terminal's environment. Credentials are not CLI arguments or log output.

The start helper waits for a new non-retained heartbeat and creates the command
automatically. Use one command operator at a time: multiple operators can choose
the same next ID, causing one command to be rejected. Broker acknowledgement only
confirms delivery to the broker; telemetry shows whether the controller acted.

**With the current sensor stubs, START should produce `FAULT` with
`sensor_invalid_or_stale`; STOP should produce `STOPPED` with `command_stop`.**
That is enough to verify command integration. Do not fake valid sensor readings
in the production loop just to make a start succeed.

## Message contract

Default root: `robot/car01`. Use a different root for each car.

| Topic suffix | Rate / QoS / retained | Payload |
| --- | --- | --- |
| `/telemetry` | Every 500 ms by default / 0 / no | JSON state, reason, requested and measured motion, line, terrain, obstacle |
| `/heartbeat` | Every 1 s / 0 / no | JSON session, uptime, last accepted command ID and error counters |
| `/command` | Subscription QoS 1; publish without retain | Strict command JSON, or plain `stop` |
| `/status` | On connect, last will / 1 / yes | `{"online":true}` or `{"online":false}` |

Read status together with heartbeat freshness. A retained online flag can be stale;
last-will detection takes time, and intentional disconnects do not trigger a will.
Treat absence of heartbeats for several seconds as unavailable. The car's MQTT
keepalive is 10 seconds; this is not an instantaneous physical emergency-stop link.

Telemetry has `schema:1`, `uptime_ms`, `state`, `reason`, target wheel speeds and
nested `motion`, `line`, `terrain`, `obstacle` objects. Sensor groups carry `valid`,
`sampled_ms` and `age_ms`. Invalid or non-finite numeric readings become JSON
`null`; boolean/event fields are meaningful only when the group's reading is valid.
Heights/profile values have additional validity flags. Units appear in field names.
`line.navigation` uses the current `Navigation` enum from `include/robot/types.h`;
navigation/barcode interpretation itself remains Buddy 3's responsibility.

Example heartbeat (values illustrative):

```json
{"schema":1,"online":true,"session":"0123456789abcdef","uptime_ms":12000,"last_command_id":0,"rejected_commands":0,"dropped_publish":0}
```

Corresponding start, sent promptly on `/command`:

```json
{"session":"0123456789abcdef","id":1,"issued_ms":12000,"command":"start"}
```

Rules:

- Exactly four fields, any order; no unknown or duplicate fields. Strings use
  plain unescaped ASCII; integer fields are unsigned 32-bit decimal integers.
- `session` must match the current heartbeat. It changes on every MQTT connection.
- `id` must be positive and greater than the last accepted ID in that session.
- `issued_ms` is **car uptime from heartbeat**, not a laptop wall-clock timestamp.
  Age must be at most 5000 ms. The start is checked again when consumed.
- A message is capped at 192 bytes. Fragments are assembled before validation.
- Stop wins over start when both are pending in the same poll. Plain `stop` is
  accepted without a session; it can never start the car.
- The session/ID rules prevent accidental replay, not unauthorized access. Broker
  access controls are responsible for who can send commands.
- Do not retain command messages. The public lwIP callback does not expose the
  retained flag; old-session starts are rejected by the application protocol.

Telemetry samples are snapshots. Very brief one-shot events between telemetry
periods may be missed; reliable event history/acknowledgements are a future extension.

## Recovery and integration

WiFi attempts have a 20-second deadline; broker connection/subscription attempts
have a 15-second deadline. Failures retry with backoff from 1 to 30 seconds. No
connection-completion wait runs inside the vehicle loop. Call `comms_poll()` often
on the same core; blocking in another buddy's module will still delay networking.

The default `ROBOT_STOP_ON_DISCONNECT=1` emits one STOP when an established MQTT
connection is lost. Initial connection failures do not repeatedly override local
USB commands. Queued starts are discarded on disconnect. Reconnection restores
the subscription and heartbeat, but never resumes driving automatically.

There is no unbounded telemetry queue: disconnected samples are dropped, and
publish-buffer failures increment `dropped_publish`. `last_command_id` acknowledges
protocol acceptance, not successful mission execution; check controller state too.

## Validation

Run `powershell -ExecutionPolicy Bypass -File scripts/check.ps1` for controller,
protocol and fake-SDK recovery tests. These cover fragmentation, malformed JSON,
overflow, replay/expiration, stop priority, stale sessions, timeouts, subscription
failure, link loss, reconnect and subscription restoration.

Run `python -m unittest discover -s tests -p "test_mqtt_client.py"` for the laptop
helper and an independent JSON parse of the C telemetry fixture (run C tests first).

Firmware was compile/link checked against Pico SDK 2.2.0 and ARM GCC 14.2.1 with
WiFi enabled and test-only credentials. Physical RF/network behavior is not yet
verified. Complete these bench checks and record evidence in `docs/test-log.csv`:

1. Observe live heartbeat and invalid-sensor telemetry on the laptop.
2. Send STOP, then START; confirm the expected controller results above.
3. Restart the broker; confirm reconnect, a new session, and working subscription.
4. Interrupt WiFi; confirm the detected-loss stop, reconnect, and no automatic start.
5. Replay an earlier start payload; confirm rejection and no state change.

Implementation references: [Pico W MQTT example](https://github.com/raspberrypi/pico-examples/tree/master/pico_w/wifi/mqtt),
[Pico polling architecture](https://www.raspberrypi.com/documentation/pico-sdk/networking.html),
[Mosquitto configuration](https://mosquitto.org/man/mosquitto-conf-5.html),
[Paho Python client](https://eclipse.dev/paho/files/paho.mqtt.python/html/client.html).
