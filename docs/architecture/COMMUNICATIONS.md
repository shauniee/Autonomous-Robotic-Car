# Communications: simple wireless S3 telemetry

## 1. Ownership and scope

Shaun / S1 owns the portable latest snapshot, mutex and JSON encoder. Nicholas / S3 supplies its local readings/control result. The existing kernel WiFi service, priority 4 on core 0, owns all CYW43, lwIP and MQTT calls. S2 remains the only motor writer. This later requested bench extension uses `SMP=1 WIFI=cyw43 NET=lwip MQTT=1`; `NET=none MQTT=0` remains available. No new application task exists.

The unchanged Word report is the integrated design proposal; full mission telemetry and validated status-request subscriptions remain deferred. This stream has **no subscriptions, echo or remote commands**. Historical schema5/dashboard files and calibration commands are not part of the active contract; the [former guide](../history/pre-mtkernel/architecture/COMMUNICATIONS.md) preserves them. Original template ownership guidance remains in [UPSTREAM_TEAM.md](../UPSTREAM_TEAM.md); see [TEAM.md](../../TEAM.md) for the requested shared-library extension and IF-05.

## 2. Files and interfaces

| File | Responsibility |
| --- | --- |
| app_program/subsystem1/telemetry.h | Portable S1Telemetry value type, update/copy/encode APIs; no RTOS/network headers |
| app_program/subsystem1/subsystem1.c | One priority-inheritance mutex and latest value; no S1 task |
| app_program/subsystem1/telemetry.c | Bounded JSON and integer-based decimal formatting |
| app_program/subsystem3/telemetry_s3.c | Convert current tracker/controller results to thousandths and labels |
| app_program/subsystem3/task_ir.c | Attempt snapshot update after each control cycle when MQTT=1 |
| lib/liblwip/sysdepend/pico_rp2040/mqtt_utk.c | Connect, copy/encode and publish within the kernel network service |
| lib/liblwip/sysdepend/pico_rp2040/lwipopts.h | MQTT output ring 1024 bytes |
| config/wifi_secrets.h | Ignored local WiFi/broker credentials; example is tracked |

`s1_telemetry_update(const S1Telemetry *)` copies a complete value and assigns its sequence. `s1_telemetry_copy(S1Telemetry *)` returns the latest coherent value if one exists. Both use `tk_loc_mtx(..., TMO_POL)` and return false on unavailable/busy access, without waiting. Access before S1 initialization safely skips diagnostics through an atomic object ID. An optional mutex allocation failure logs once and leaves local startup available. S3 allocates no extra kernel object.

The latest value replaces the previous value; there is no offline queue. S3's float calculations are converted to signed thousandths in the adapter. The encoder emits three decimal places through bounded integer formatting, without `%f` or the T-Monitor formatter. The source of truth for motion remains the S3 controller and S2 request/health interfaces.

## 3. MQTT and payload contract

Topic `robot/s3/telemetry`, client ID `csc01-s3-pico`, TCP port 1883, QoS **0**, retain **false**, nominal period **500 ms**. An accepted connection attempts the latest snapshot immediately, then starts the periodic timer. The payload limit is **768 bytes** plus one local NUL byte; the MQTT output ring is **1024 bytes**. Oversized/unterminated input, unavailable snapshot or publication error skips that message. QoS 0 is observation without delivery guarantees.

| JSON field | Meaning / unavailable form |
| --- | --- |
| schema | 1; separate from historical schema5 |
| sequence | Successful S1 snapshot update count, wraps as uint32; reset at reboot; normally jumps between publications |
| observed_ms | S3 snapshot time in boot milliseconds; it is not acquisition time |
| sampled_ms | Original latest acquisition-attempt timestamp; null before first attempt |
| sample_age_ms | Publication-time minus sampled_ms using uint32 subtraction; null before first attempt; works across wrap |
| acquisition_valid | Latest paired ADC acquisition succeeded; distinct from calibration and freshness |
| sample_fresh | Successful acquisition and age <=60 ms |
| raw | ADC left/right 0..4095 object; null before first/after failed acquisition |
| calibration.black_set / white_set | Completed reference flags, RAM-only |
| calibration.black / white | Paired ADC reference object, or null if its flag is false |
| fractions | Calibrated left/right black fractions 0..1, or null without a valid calibrated reading |
| error | Right fraction minus left fraction, or null without calibrated reading |
| line | unknown, visible, lost or broad_black; see the complete [IR catalogue](LINE_FOLLOWING.md#5-complete-ir-state-catalogue) |
| state | READY, FOLLOW_LINE, RECOVER_LINE or STOPPED |
| reason | Current controller reason, including capture/start/stop/failure policy |
| capture | Typed capture outcome's display name; not a remote acknowledgement or request ID |
| requested_duty.left / right | S3 requested duties; not measured motor output/speed |
| motion_enabled | S3 following/recovery request permission |
| test_inhibited | True in TEST firmware; S2 brakes even when motion_enabled is true |

Stale historical fractions can still be present; assess them with `sample_fresh`, acquisition validity and age. Telemetry never renews acquisition time. `null` means unavailable, rather than a made-up zero reading. A plausible ADC value cannot establish sensor connection. The 500 ms stream can miss a whole 100 ms recovery turn and cannot prove stop/recovery timing.

## 4. Reconnection and control isolation

The service uses one periodic publish callback and one retry callback, with at most one of these adapter timers pending. Each connection status cancels old adapter timers. A refused/disconnected/immediately failed connect schedules retry after 3000 ms. The service's frequent polling does not bypass the retry delay or start a second connection; invalid IPv4 configuration is diagnosed once. Existing lwIP/MQTT protocol timers remain owned by the stack. Publish failures skip the message while the connected periodic stream continues; an asynchronous disconnect enters the retry path.

Reconnect sends the newest stored snapshot, not a backlog. No MQTT state transition calls S2 or S3 policy functions. Snapshot copying is nonblocking, the network stays on core 0, GP21 processing stays in the core-1 control loop, and the watchdog still requires advancing valid ADC and Motion progress. Healthy local following continues through network loss. Physical scheduling/stack margin under network load still needs measurement.

## 5. Mac operation and evidence

Use [getting started section 5](../GETTINGS_STARTED.md#5-wireless-telemetry-on-the-mac) for the ignored credentials, broker/subscriber commands and exact normal/TEST build flags. USB alone is supported by [Cytron](https://my.cytron.io/tutorial/get-started-robo-pico-circuitpython-power-up-robo-pico). Verify wireless TEST connectivity over USB alone first; unplug USB before power-bank normal trials, then recalibrate using GP20/21 and GP22 cues. Keep the Mac on the reachable private 2.4 GHz network; this anonymous non-TLS bench broker is not an internet service.

## 6. Test cases

| IDs | Scope | Evidence level |
| --- | --- | --- |
| S1-U04/U05 | Encoding bounds, nulls, signed decimals, original age/wrap, coherent snapshots and skipped access | Host fakes; U04 also compiled into TEST image |
| INT-06 | Broker refusal, disconnect/reconnect, publish failures, latest snapshot, timer count | Real adapter with fake lwIP/kernel/clock |
| INT-07 | S3 controls/watchdog/timestamps with available or failing telemetry, both TEST profiles | Real S3 loop with fake hardware/S2/kernel |
| INT-08 | Encoded fixture through actual Mosquitto CLI subscriber; broker restart and no retention | Mac loopback transport only |
| SYS-09/10 | Actual Pico USB-alone TEST then bank-alone normal telemetry and network loss | On-target/physical; pending |

## 7. Results

Results and saved logs live in [S1](../../testing/subsystem1/TEST_LOG.md), [integration](../../testing/integration/TEST_LOG.md) and [system](../../testing/system/TEST_LOG.md). Host/build/local broker evidence is recorded separately from actual Pico WiFi and motor trials. Report T12/T13 full mission telemetry/reconnect trials remain pending.
