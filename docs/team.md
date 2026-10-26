# Team, hardware and workflow

Single owner of: members, subsystem ownership, pins and wiring, task/kernel-object budget, cross-subsystem interfaces and coding rules. Build, flash and bring-up: [README](../README.md#getting-started). Requirements, data widths and pending decisions: [project-specs](project-specs.md). Shared-data rules and MQTT frame: [architecture](architecture.md).

A **subsystem** is one student's folder (may hold several tasks and peripherals); a **task** is a thread of execution; a **peripheral** is a hardware block. Only the owner edits a subsystem's folders; request the owner's review for other changes. All members join integration, debugging, system testing and demonstration.

## 1. Members and subsystems

| # | Subsystem | Student ID | GitHub | Code | Tests | Guide |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | **Shaun, Communications:** WiFi/MQTT, telemetry, reporting | 2500629 | shauniee | [app_program/subsystem1/](../app_program/subsystem1/) | [testing/subsystem1/](../testing/subsystem1/) | [subsystem1](architecture/subsystem1.md) |
| 2 | **Mahesha, Motion:** motors, encoders, wheel PID, motion APIs | 2503973 | itsjustmdawg | [app_program/subsystem2/](../app_program/subsystem2/) | [testing/subsystem2/](../testing/subsystem2/) | [subsystem2](architecture/subsystem2.md) |
| 3 | **Nicholas, Line, Barcode, Mission/Safety:** IR, barcode, coordination, integration | 2501552 | nicholas-bay | [app_program/subsystem3/](../app_program/subsystem3/) | [testing/subsystem3/](../testing/subsystem3/) | [subsystem3](architecture/subsystem3.md) |
| 4 | **Darrius, Terrain:** IMU processing, motion classification, hump measurement | 2500360 | Galaxius03 | [app_program/subsystem4/](../app_program/subsystem4/) | [testing/subsystem4/](../testing/subsystem4/) | [subsystem4](architecture/subsystem4.md) |
| 5 | **Jia Jing, Obstacle:** sonar, servo scanning, obstacle profiling | 2502825 | JJLOLLOL | [app_program/subsystem5/](../app_program/subsystem5/) | [testing/subsystem5/](../testing/subsystem5/) | [subsystem5](architecture/subsystem5.md) |

Stable test IDs/measurements: [testing](../testing/README.md). Demonstrations: [videos](../videos/README.md). Course/calibration decisions are recorded once in [project-specs](project-specs.md#pending-decisions).

**Current capabilities (software state, not physical proof).** S3 reads/calibrates two IR sensors and produces P steering; a separate line-following task uses the S2 request/stop/health interface (physical baseline pending). Optional S1 reporting gives telemetry, heartbeat and status-only subscription via the kernel service (S3 adapter pending). Barcode buffering, provisional A-Z decoding and mapping/event APIs are implemented but **boot disabled** without measured configuration and an encoder/GPIO provider. Pending: encoder feedback, physical barcode capture, IMU/sonar acquisition, full Mission/Safety. Compilation or retained software evidence does not establish a physical tracking run, barcode scan or verified motor operation.

## 2. Pins

| Pin | Owner | Used for |
| --- | --- | --- |
| GP23, GP24, GP25, GP29 | Kernel (reserved) | Pico W radio/internal signals; never allocate externally |
| GP0 | Mahesha | Right encoder B, Grove 1, green; UART0 console/crash conflict |
| GP1 | Mahesha | Right encoder A, Grove 1, red; UART0 console/crash conflict |
| GP2 | Jia Jing | Reported HC-SR04P Echo, Grove 2; shares Maker port |
| GP3 | Jia Jing | Reported HC-SR04P Trig, Grove 2; shares Maker port |
| GP4 | Darrius | GY-511 SDA, Grove 3 / I2C0 |
| GP5 | Darrius | GY-511 SCL, Grove 3 / I2C0 |
| GP6, GP13, GP14, GP15, GP19 | Unclaimed | Check carrier sharing before allocation |
| GP7 | Mahesha | Left encoder B, Grove 7, green |
| GP8 / GP9 | Mahesha | Left motor M1A (black) / M1B (yellow); exclusive motor writer |
| GP10 / GP11 | Mahesha | Right motor M2A (black) / M2B (yellow); exclusive motor writer |
| GP12 | Jia Jing | SG90 Servo 1 signal S, orange; bench holds signal low |
| GP16 | Nicholas | Barcode IR DOUT, Grove 4; AOUT disconnected |
| GP17 | Unclaimed | Grove 4 second signal unused |
| GP18 | Unclaimed | Onboard RGB LEDs; no firmware owner |
| GP20 | Nicholas | Onboard active-low capture/start/reset; proposed Safety input |
| GP21 | Nicholas | Onboard active-low stop/cancel; independent stop path |
| GP22 | Nicholas | Onboard PWM buzzer; mute switch controls audibility |
| GP26 | Nicholas | Left line IR AOUT, Grove 5 / ADC0; disconnect Grove 6 shared GP26 contact |
| GP27 | Nicholas | Right line IR AOUT, Grove 6 / ADC1 |
| GP28 | Mahesha | Left encoder A, Grove 7, red; ADC2-capable but used digitally; open battery jumper |

### Supply and wires

- Power bank: red to green terminal VIN+, black to VIN-. Verify polarity.
- Encoders: white to 3V3, blue to GND. Line/barcode IR: VCC to 3V3, GND to GND; both line DOUT wires stay disconnected.
- GY-511: Vin to 3V3, GND to GND; separate `3.3`, INT1, INT2, DRDY disconnected. Reported HC-SR04P: VCC to 3V3, GND to GND; verify variant and Echo HIGH before connection.
- SG90: red to +, brown to -; verify carrier silkscreen and supply rail.

### Wiring conflicts

- Grove 5 and Grove 6 share GP26; joining analogue outputs is unsafe, keep the Grove 6 GP26 contact disconnected.
- GP28 battery sensing stays disconnected via its jumper while GP28 is the left encoder.
- UART0 console/crash reporting claim GP0/GP1: use `CONSOLE=usb_cdc CRASH_REPORT=0` with encoders attached.
- Grove 2 and the Maker port share GP2/GP3; do not attach independent devices to both.
- Disconnect GY-511 wires reused elsewhere before restoring I2C; its separate `3.3` pin is not established as an extra supply input.
- Line sensing and digital-encoder GP28 consume the external ADC-capable pins; barcode AOUT needs a reviewed allocation change or extra hardware.

### Physical header map (W/WH; header numbers differ from GPIO numbers)

| Pin | Signal | Pin | Signal | Pin | Signal | Pin | Signal |
| ---: | --- | ---: | --- | ---: | --- | ---: | --- |
| 1 | GP0 | 11 | GP8 | 21 | GP16 | 31 | GP26 / ADC0 |
| 2 | GP1 | 12 | GP9 | 22 | GP17 | 32 | GP27 / ADC1 |
| 3 | GND | 13 | GND | 23 | GND | 33 | AGND |
| 4 | GP2 | 14 | GP10 | 24 | GP18 | 34 | GP28 / ADC2 |
| 5 | GP3 | 15 | GP11 | 25 | GP19 | 35 | ADC_VREF |
| 6 | GP4 | 16 | GP12 | 26 | GP20 | 36 | 3V3 OUT |
| 7 | GP5 | 17 | GP13 | 27 | GP21 | 37 | 3V3_EN |
| 8 | GND | 18 | GND | 28 | GND | 38 | GND |
| 9 | GP6 | 19 | GP14 | 29 | GP22 | 39 | VSYS |
| 10 | GP7 | 20 | GP15 | 30 | RUN | 40 | VBUS |

## 3. Task priorities and kernel object budget

Smaller number = higher priority. Priorities 1-4 are kernel (initial task 1, USB 3, WiFi 4, all core 0). `TP_PRC1` core 0; `TP_PRC2` core 1; 0 any. Application tasks use core 1 in SMP builds.

**Current bench**

| Subsystem | Tasks (name - priority - core) |
| --- | --- |
| Shaun | No application task; optional snapshot mutex only |
| Mahesha | `motion_task` - 13 - core 1; 20 ms or stop wake-up |
| Nicholas line/barcode | `task_s_task` - 12 - core 1; buttons 10 ms, paired ADC 20 ms |
| Nicholas bench coordination | `line_follow_task` - 14 - core 1; local controls, snapshot-to-Motion requests every 10 ms |
| Darrius, Jia Jing | Inactive |

**Proposed full robot** (team acceptance not recorded)

| Subsystem | Tasks (name - priority - core) |
| --- | --- |
| Shaun | Reporting bridge - 20 - core 1; network events / reporting interval |
| Mahesha | Motion - 13 - core 1; 20 ms / stop |
| Nicholas | Safety - 12 - core 1, 10 ms / events; Mission - 14 - core 1, 20 ms / events; Line and Barcode - 15 - core 1, ADC 20 ms / barcode IRQ |
| Darrius | Terrain - 16 - core 1; 20 ms fresh-data polling |
| Jia Jing | Obstacle - 17 - core 1; bounded acquisition states |

Record every new task; agree priority changes before integration. Kernel network priority stays 4.

**Kernel object budget** (limits in [config/config.h](../config/config.h) are total capacities; current application allocations exclude kernel/driver objects; changes need shared-config agreement)

| Object | Limit | Shaun | Mahesha | Nicholas | Darrius | Jia Jing |
| --- | --- | --- | --- | --- | --- | --- |
| Task | 32 | 0 | 1 | 2 | 0 | 0 |
| Event flag | 16 | 0 | 1 | 0 | 0 | 0 |
| Mutex | 12 | 0 or 1 | 1 | 2 | 0 | 0 |
| Semaphore 16; mailbox, message buffer, fixed pool, variable pool, cyclic, alarm handler 12 each | - | 0 | 0 | 0 | 0 | 0 |

Shaun attempts one mutex with `MQTT=1`, none with `MQTT=0`; allocation failure disables optional diagnostics and permits local startup. Mahesha's request and health copies share one mutex; persistent stop uses one event flag. Nicholas owns two private mutexes (line snapshot; barcode service/configuration/event FIFO). Barcode uses static 64-edge and eight-event storage, no message buffer or extra task. Both S3 tasks use the existing 2048-byte stack; barcode stack/timing margins unmeasured. Create all subsystem objects before starting application tasks; required init failure prevents startup, optional telemetry failure may skip reporting. Measure stack margin and total object usage (including kernel services) before the proposed task split.

## 4. Interfaces between subsystems

The provider creates and owns each shared object; objects stay private behind public APIs. Agree new formats/ownership before coding. Team agreement is not recorded for any row.

| ID | From to To | Mechanism | Object / API | Data / meaning |
| --- | --- | --- | --- | --- |
| IF-01 | Nicholas to Mahesha | Latest value under Mahesha priority-inheritance mutex | `request_mutex` (private); `s2_publish()` | `MotionRequest`: duties, enabled flag, original sample time |
| IF-02 | Nicholas to Mahesha | Persistent event flag + wake-up, independent of request mutex | `stop_flag`, `motion_tid` (private); `s2_stop()` / `s2_reset_stop()` | Stop latch; fresh local reset invalidates old request |
| IF-03 | Mahesha to Nicholas | Coherent health copy under mutex; stop flag read separately | `s2_health()` | `MotionHealth`: software service progress and latch; no physical brake/stationarity acknowledgement |
| IF-04 | IR tracker to Nicholas coordinator | In-task typed value; no kernel object | `ir_tracking_output()` | `IrTrackingOutput`: validity, line condition, signed error/steering, sample time |
| IF-05 | Nicholas to Shaun to kernel network | Latest value under optional Shaun mutex; `TMO_POL` both sides | `telemetry_mutex` (private); `s1_telemetry_update()` / `s1_telemetry_copy()` | `S1Telemetry`; busy/unavailable skips diagnostics |
| IF-06 | Unclaimed | - | - | - |

IF-01 to IF-04 limits, expiry and ordering: [subsystem2](architecture/subsystem2.md) and [subsystem3](architecture/subsystem3.md). IF-05: [subsystem1](architecture/subsystem1.md). Full-robot message/queue contracts are proposed in [architecture](architecture.md) without allocated object IDs. Each cross-subsystem interface needs an [integration test record](../testing/integration/TEST_LOG.md); implementation or a saved software result does not establish physical acceptance.

## 5. Coding rules

Apply to new or changed application code (imported kernel need not be reformatted). Informed by Barr Group 2018 embedded C guidance; not a compliance certification.

- **Language/layout:** GNU C11 (`-std=gnu11`). Use C11 acquire/release for word snapshots and startup publication; no atomic RMW helpers on Cortex-M0+. Isolate other extensions in drivers. Four spaces, LF, one statement per line, braces on every body, opening brace on its own line, ~80 columns. Space operators, parenthesize precedence, comment purpose/units/bounds/assumptions, remove dead code.
- **Modules:** one purpose each; lowercase `snake_case.c/.h`, guarded header, minimal exports; never include a `.c` or absolute path. Public functions prefixed with module name; private `static`. Named constants; prefer functions to macros (macros evaluate args once, parenthesized). Types `module_name_t`.
- **Data:** fixed-width ints where width matters; packed bits for stored/shared flags; `bool` only for locals/returns; `const` for immutable. Consistent unsigned literals; justify narrowing casts; avoid signed bit ops. Initialize before use; explicit invalid codes. `float` only for justified local computation (`f` suffix, finite/range checks); stored/shared values are fixed point with documented scale (widths and scales in [project-specs](project-specs.md)). In-RAM packed state may use bit-fields; wire formats (MQTT) use explicit masks/shifts with documented byte order, never native structs, padding or bit-field order.
- **Minimum-bit:** each field gets `ceil(log2(values))` bits including the invalid code; record range, unit, signedness, scale, rounding, invalid code, overflow behavior in project-specs before converting. Exclusive states use one encoded value; coexisting faults use independent bits. Combine fields in one unit only with a shared writer. Access via `module_field_get()/_set()`; no caller masks. Loop counters/temporaries exempt. Test min, max and invalid code; overflow saturates or faults as documented.
- **Control:** check return codes; safe failure responses; `default` in switches; document fallthrough. Bound polling, waits, retries, buffers, actions; validate counts/indices/inputs; keep acquisition time; wrap-safe elapsed time.
- **Concurrency:** `volatile` is for hardware/async access, not synchronization. Share data only via the three patterns in [architecture](architecture.md): one-word snapshot (single store/load), mutex copy above 32 bits, ISR-to-task ring. One writer per storage unit; only it does RMW. Packed data shared with a core-1 ISR updates inside `DI`/`EI`; packed data written from both cores needs a mutex or RP2040 hardware spinlock (`DI` masks only the local core). ISRs: capture, signal, return; no blocking, formatting, decoding, allocation; use `_task`/`_isr` names.
- **Ownership:** one motor writer, separate peripheral owners, kernel-only networking; Stop must not depend on diagnostics queues or the request mutex. No credentials in version control.

**Review checklist:** purpose/API/units/ownership clear, no duplicate mutable state; style consistent; inputs, casts, arithmetic, finite values, bounds checked; hardware actions bounded, failures inhibit unsafe output; shared values synchronized, cached observations keep acquisition time; stop, stale data and reset keep the contract; no ISR blocking, foreign motor writes, student network calls or committed credentials; packed fields match project-specs, use accessors, have boundary tests; one writer per storage unit; evidence names source/profile (a build is not hardware proof).
