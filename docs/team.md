# Team, hardware and workflow

## Contents

- [1. Members and subsystems](#1-members-and-subsystems)
- [2. Pins](#2-pins)
  - [Supply and wires](#supply-and-wires)
  - [Wiring conflicts](#wiring-conflicts)
  - [Physical header map (W/WH; header numbers differ from GPIO numbers)](#physical-header-map-wwh-header-numbers-differ-from-gpio-numbers)
- [3. Task priorities and kernel object budget](#3-task-priorities-and-kernel-object-budget)
- [4. Interfaces between subsystems](#4-interfaces-between-subsystems)
- [5. Coding rules](#5-coding-rules)

Single owner of: members, subsystem ownership, pins/wiring, current kernel-object inventory and coding rules. Architecture owns target scheduling and cross-subsystem contracts. Build, flash and bring-up: [README](../README.md#getting-started). Requirements, data widths and pending decisions: [project-specs](project-specs.md). Shared-data rules and MQTT frame: [architecture](architecture.md).

A **subsystem** is one student's folder (may hold several tasks and peripherals); a **task** is a thread of execution; a **peripheral** is a hardware block. Subsystem owners retain responsibility for their folders; coordinate firmware changes with the owner. This documentation reconciliation is authorized by Nicholas. All members join integration, debugging, system testing and demonstration.

## 1. Members and subsystems

| # | Subsystem | Student ID | GitHub | Code | Tests | Guide |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | **Shaun, Communications:** WiFi/MQTT, telemetry, reporting | 2500629 | shauniee | [app_program/subsystem1/](../app_program/subsystem1/) | [testing/subsystem1/](../testing/subsystem1/) | [subsystem1](architecture/subsystem1.md) |
| 2 | **Mahesha, Motion:** motors, encoders, wheel PID, motion APIs | 2503973 | itsjustmdawg | [app_program/subsystem2/](../app_program/subsystem2/) | [testing/subsystem2/](../testing/subsystem2/) | [subsystem2](architecture/subsystem2.md) |
| 3 | **Nicholas, Line, Barcode, Mission integration:** IR, barcode, coordination, integration | 2501552 | nicholas-bay | [app_program/subsystem3/](../app_program/subsystem3/) | [testing/subsystem3/](../testing/subsystem3/) | [subsystem3](architecture/subsystem3.md) |
| 4 | **Darrius, Terrain:** IMU processing, motion classification, hump measurement | 2500360 | Galaxius03 | [app_program/subsystem4/](../app_program/subsystem4/) | [testing/subsystem4/](../testing/subsystem4/) | [subsystem4](architecture/subsystem4.md) |
| 5 | **Jia Jing, Obstacle:** sonar, servo scanning, obstacle profiling | 2502825 | JJLOLLOL | [app_program/subsystem5/](../app_program/subsystem5/) | [testing/subsystem5/](../testing/subsystem5/) | [subsystem5](architecture/subsystem5.md) |

Stable test IDs/measurements: [testing](../testing/README.md). Demonstrations: [week13-plan](week13-plan.md#4-video-recording). Course/calibration decisions are recorded once in [project-specs](project-specs.md#pending-decisions).

**Implementation status.** The current source baseline and consolidated issue
plan are linked from [plan](https://github.com/shauniee/Autonomous-Robotic-Car/issues/117#status). The code
contains Motion PID/actions, Mission/Safety, line/barcode, Terrain, Obstacle and
Reporting modules; this is not physical acceptance. Older smaller bench/task
arrangements are historical. Target coordination is in [architecture](architecture.md).
Nicholas reviews this documentation plan; subsystem responsibilities remain.

## 2. Pins

| Pin | Owner | Used for |
| --- | --- | --- |
| GP23, GP24, GP25, GP29 | Kernel (reserved) | Pico W radio/internal signals; never allocate externally |
| GP0 | Mahesha | Right encoder B, Grove 1, green; UART0 console/crash conflict |
| GP1 | Mahesha | Right encoder A, Grove 1, red; UART0 console/crash conflict |
| GP2 | Jia Jing | Reported HC-SR04P Trig, Grove 2; shares Maker port |
| GP3 | Jia Jing | Reported HC-SR04P Echo, Grove 2; shares Maker port |
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
| GP20 | Nicholas | Onboard active-low calibration/start; target fresh start only after eligible reset |
| GP21 | Nicholas | Onboard active-low stop/cancel; target eligible release-to-ready handshake |
| GP22 | Nicholas | Onboard PWM buzzer; mute switch controls audibility |
| GP26 | Nicholas | Left line IR DOUT, Grove 5 / digital input; disconnect Grove 6 shared GP26 contact |
| GP27 | Nicholas | Right line IR DOUT, Grove 6 / digital input |
| GP28 | Mahesha | Left encoder A, Grove 7, red; ADC2-capable but used digitally; open battery jumper |

User confirmation 2026-10-29: physical wiring matches this allocation and sonar electrical behavior is confirmed; motor/encoder polarity is not confirmed. This does not append a measured test result.

### Supply and wires

- Power bank: red to green terminal VIN+, black to VIN-. Verify polarity.
- Encoders: white to 3V3, blue to GND. Line/barcode IR: VCC to 3V3, GND to GND; connect line DOUT to GP26/GP27; both line AOUT wires stay disconnected.
- GY-511: Vin to 3V3, GND to GND; separate `3.3`, INT1, INT2, DRDY disconnected. Reported HC-SR04P: VCC to 3V3, GND to GND, Trig to GP2, Echo to GP3; verify variant and Echo HIGH (at most 3.3 V) before connection.
- SG90: red to +, brown to -; verify carrier silkscreen and supply rail.

### Wiring conflicts

- Grove 5 and Grove 6 share GP26; joining independent sensor outputs is unsafe, keep the Grove 6 GP26 contact disconnected.
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

[Architecture](architecture.md#tasks) owns the only target task/core/scheduling
table. [robot_config.h](../app_program/common/robot_config.h) contains current
provisional task settings. Keep Motion/Mission on core 1 and I/O on core 0;
no core swap. Existing priorities/periods are retained provisionally with user authorization;
D13 physical scheduling acceptance remains pending.
`TP_PRC1` means core 0 and `TP_PRC2` core 1; lower priority number runs first.

Working software allocation inventory after #130–#133 (excluding kernel/driver
allocations; not measured live high-water marks):

| Application object | Allocation / ownership |
| --- | --- |
| Tasks | Motion and Mission on core 1, I/O on core 0: exactly three creation call sites |
| Mutexes | Five: Motion command/health, line snapshot, S5 profile, cross-core events, Terrain evidence |
| Event flags | One persistent Motion stop/inhibit flag |
| Static rings | Existing barcode capture/events and S1 bridge/events; no generic bus |
| Task stacks | 2048 bytes requested each, 6144 bytes total requested; final stack margins unmeasured |

Limits in [config/config.h](../config/config.h) are total capacities, including
kernel/driver objects. Recount actual allocations and measure memory after task
consolidation; no invented savings or margin. Required control initialization
failure inhibits startup. Optional data failure is handled by the central policy;
normal mission readiness still requires operational WiFi and MQTT.

## 4. Interfaces between subsystems

Each provider owns private storage and exposes coherent accessors. Current APIs
are inventoried in subsystem guides; target authority/sharing/freshness is owned
by [architecture](architecture.md#shared-data-rules).

| Path | Ownership / target rule |
| --- | --- |
| Mission to Motion | One latest packed command, bounded mutex copy; Motion owns feedback and stop latch |
| Local GP21 to Motion | Independent input check, persistent stop; no command-mutex/telemetry dependency |
| S3 evidence to Mission | Bounded in-task sensing/decoder steps; retain original acquisition times |
| Mission to S5 | Scan/side-watch intent and bounded acquisition steps, never a second motor writer |
| Terrain to Mission/Motion | Core-0 coherent evidence with age/validity; no cross-core runtime `s2_stop()` call |
| I/O to Mission mapping | Bounded hand-off; Mission installs whole mapping while stopped and returns result before ACK |
| Mission/Terrain to Reporting | Cross-core event synchronization required; same-core DI/EI is insufficient |
| Reporting to kernel MQTT | Opaque bounded TX/RX bridge; only WiFi context invokes networking APIs |

Record changed interface layouts before coding and append integration results in
[testing/integration/TEST_LOG.md](../testing/integration/TEST_LOG.md). A valid
software copy or brake acknowledgement is not physical stationarity.

## 5. Coding rules

Apply to new or changed application code (imported kernel need not be reformatted). Informed by Barr Group 2018 embedded C guidance; not a compliance certification.

- **Language/layout:** GNU C11 (`-std=gnu11`). Use C11 acquire/release for word snapshots and startup publication; no atomic RMW helpers on Cortex-M0+. Isolate other extensions in drivers. Four spaces, LF, one statement per line, braces on every body, opening brace on its own line, ~80 columns. Space operators, parenthesize precedence, comment purpose/units/bounds/assumptions, remove dead code.
- **Modules:** one purpose each; lowercase `snake_case.c/.h`, guarded header, minimal exports; never include a `.c` or absolute path. Public functions prefixed with module name; private `static`. Named constants; prefer functions to macros (macros evaluate args once, parenthesized). Types `module_name_t`.
- **Data:** fixed-width ints where width matters; packed bits for stored/shared flags; `bool` only for locals/returns; `const` for immutable. Consistent unsigned literals; justify narrowing casts; avoid signed bit ops. Initialize before use; explicit invalid codes. `float` only for justified local computation (`f` suffix, finite/range checks); stored/shared values are fixed point with documented scale (widths and scales in [project-specs](project-specs.md)). In-RAM packed state may use bit-fields; wire formats (MQTT) use explicit masks/shifts with documented byte order, never native structs, padding or bit-field order.
- **Minimum-bit:** each field gets `ceil(log2(values))` bits including the invalid code; record range, unit, signedness, scale, rounding, invalid code, overflow behavior in project-specs before converting. Exclusive states use one encoded value; coexisting faults use independent bits. Combine fields in one unit only with a shared writer. Access via `module_field_get()/_set()`; no caller masks. Loop counters/temporaries exempt. Test min, max and invalid code; overflow saturates or faults as documented.
- **Control:** check return codes; safe failure responses; `default` in switches; document fallthrough. Bound polling, waits, retries, buffers, actions; validate counts/indices/inputs; keep acquisition time; wrap-safe elapsed time.
- **Concurrency:** `volatile` is for hardware/async access, not synchronization. Share data only via the three patterns in [architecture](architecture.md): one-word snapshot (single store/load), mutex copy above 32 bits, ISR-to-task ring. One writer per storage unit; only it does RMW. Packed data shared with an ISR on the same core updates inside `DI`/`EI`; packed data written from both cores needs a mutex or RP2040 hardware spinlock (`DI` masks only the local core). ISRs: capture, signal, return; no blocking, formatting, decoding, allocation; use `_task`/`_isr` names.
- **Ownership:** one motor writer, separate peripheral owners, kernel-only networking; Stop must not depend on diagnostics queues or the request mutex. No credentials in version control.

**Review checklist:** purpose/API/units/ownership clear, no duplicate mutable state; style consistent; inputs, casts, arithmetic, finite values, bounds checked; hardware actions bounded, failures inhibit unsafe output; shared values synchronized, cached observations keep acquisition time; stop, stale data and reset keep the contract; no ISR blocking, foreign motor writes, student network calls or committed credentials; packed fields match project-specs, use accessors, have boundary tests; one writer per storage unit; evidence names source/profile (a build is not hardware proof).
