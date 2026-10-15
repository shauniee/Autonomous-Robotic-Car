# Subsystem 1: Communications

Owner: Shaun. Reporting copies actual Mission/S2/S3/S4/S5 snapshots every nominal 100 ms. The existing core-0 kernel service exclusively owns WiFi/lwIP/MQTT; student tasks do not call network APIs. Busy/unavailable reporting never authorizes movement or participates in watchdog health.

## MQTT contract

Default root is `robot/s3`, port 1883. Topics are independently configurable, distinct, nonempty, below 96 bytes and without wildcards. All seven topics are validated.

| Suffix | Payload / behavior |
| --- | --- |
| `/telemetry` | Schema 3 actual robot snapshot; QoS0, nonretained, nominal 500 ms |
| `/heartbeat` | Schema 1 bounded network diagnostics; nominal one second |
| `/status` | Retained presence/last will; a requested schema-3 robot snapshot is nonretained |
| `/request` | Read-only status or event retrieval; movement/start/reset/calibration rejected |
| `/mapping` | Nonretained `barcode_mapping` configuration with ID and 26 ordered A-Z entries |
| `/mapping_status` | QoS1 nonretained result with request ID, result and revision |
| `/events` | Nonretained schema-3 bounded accepted-event history |

The helper's four-letter JSON file is local input, not firmware storage. It expands to `{"request":"barcode_mapping","request_id":1,"mapping":[26 action strings]}`. Exactly four distinct letters have one each of left/right/straight/u_turn; other entries are unmapped. The mapping parser accepts reordered object fields, rejects duplicates/unknown fields, invalid enums/IDs, wrong counts, NUL, incomplete fragments and payloads above 512 bytes. IDs advance as unsigned serial numbers within a boot.

Mission revalidates the table on core 1 while stopped/stationary. Result values are 0 none, 1 pending, 2 applied, 3 busy, 4 invalid. Applied waits for the provider's mapping revision. A four-result queue reserves storage before submitting; a full queue does not apply configuration. Failed enqueue/delivery returns the same result to retry without replaying configuration; old completion tokens cannot clear a new transmission.

Mapping remains RAM-only and cannot start/reset motion. Reload after reboot. Publish with retain=false; the trusted broker must not retain configuration. Its retained-message policy is an acceptance prerequisite, not a firmware authentication mechanism.

Status request is `{"request":"status"}`. Event request is `{"request":"events","kind":"hump","after_id":0}` or kind `barcode`; read-only field order is fixed, cursor is unsigned, and the whole request is bounded to 64 bytes. Requests are rate limited to one per second. The helper subscribes first, ignores retained replies and requires schema 3 for robot status.

## Snapshots and event history

Maximum wire payload is 1536 bytes; MQTT output ring is 2048 bytes. Encoders are bounded; failed serialization skips the message. Missing/stale readings are null, and original acquisition age/clock is retained. Motion distances/sonar/heights are millimetres, speed mm/s, angles/rates millidegrees, acceleration mg, magnetic readings milli-gauss, IR/duty decimal thousandths. Schema 3 uses `ms` for group acquisition timestamps.

Each producer retains eight accepted records, with run-scoped count, event ID, dropped-history count and saturation flag. Scanner rejection/reset does not clear accepted barcode records. Stop preserves accepted hump data; a new run clears it coherently. Periodic telemetry carries counts, latest accepted record tuples and last symbol/action; the highest accepted height remains independent of an invalid current candidate. Event requests recover retained records even when individual telemetry snapshots were missed.

Event replies contain schema, kind, run, after_id, count, dropped, overflow and oldest-first records newer than the cursor. Each record is `[id, sampled_at, revision, a, b, value, detail]`. Humps use kernel milliseconds, x/y millimetres, height millimetres and zero revision/detail. Barcodes use hardware TIMER microseconds, signed wrapping left/right counts, decoded character code, mapping revision and `detail=(action<<1)|orientation`. Do not divide a wrapping microsecond counter into kernel boot time. Reset the cursor when run changes. History eviction and counter saturation are explicit; this is not persistent storage.

## Lifecycle and verification

Initialize -> WiFi -> MQTT subscriptions -> Online; failure uses a 15-second attempt deadline and bounded 1-30-second backoff. Disconnect discards incomplete requests, retries outstanding acknowledgements and publishes newest observations after recovery. Healthy local autonomy continues through network loss.

Modules: `telemetry_robot`, `telemetry`, `status_protocol`, `mapping_protocol`, `mapping_ack`, lifecycle `subsystem1`, and kernel `mqtt_utk`. [GUIDE](../../GUIDE.md) owns operator commands; [S1 tests](../../testing/subsystem1/TEST_LOG.md) own cases. A compiler/helper check is not Pico-originated MQTT evidence.
