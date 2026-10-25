# Team and subsystems

This guide records who owns each subsystem, pin, peripheral, task and kernel object. A subsystem is one student's part of the robot, with its own folder; it may contain several tasks and peripherals.

Only the owner edits a subsystem's folders. Request the owner's review for changes to another subsystem. All members participate in integration, debugging, system testing and demonstration.

## 1. Members and subsystems

| # | Subsystem (what it does) | Student ID | GitHub username | Code | Tests |
| --- | --- | --- | --- | --- | --- |
| 1 | **Shaun — Communications:** WiFi/MQTT integration, telemetry and reporting | 2500629 | shauniee | [app_program/subsystem1/](../app_program/subsystem1/) | [testing/subsystem1/](../testing/subsystem1/) |
| 2 | **Mahesha — Motion:** motors, encoders, wheel PID and motion APIs | 2503973 | itsjustmdawg | [app_program/subsystem2/](../app_program/subsystem2/) | [testing/subsystem2/](../testing/subsystem2/) |
| 3 | **Nicholas — Line and Barcode:** IR, barcode, Mission/Safety coordination and integration | 2501552 | nicholas-bay | [app_program/subsystem3/](../app_program/subsystem3/) | [testing/subsystem3/](../testing/subsystem3/) |
| 4 | **Darrius — Terrain:** IMU processing, motion classification and hump measurement | 2500360 | Galaxius03 | [app_program/subsystem4/](../app_program/subsystem4/) | [testing/subsystem4/](../testing/subsystem4/) |
| 5 | **Jia Jing — Obstacle:** sonar, servo scanning and obstacle profiling | 2502825 | JJLOLLOL | [app_program/subsystem5/](../app_program/subsystem5/) | [testing/subsystem5/](../testing/subsystem5/) |

Use development branches and commit prefixes `feat:`, `chore:` or `fix:`. Follow [CODING_STANDARD](CODING_STANDARD.md), record stable test IDs and measurements in [testing](../testing/README.md), and demonstrations in [videos](../videos/README.md). Record course/calibration decisions once in [PROJECT_SPECS](PROJECT_SPECS.md#pending-decisions), then update the owning guide.

## 2. Pins
 [TECHNICAL_INFO](TECHNICAL_INFO.md) owns electrical limits and connector definitions; [GETTING_STARTED](GETTING_STARTED.md#current-capabilities) identifies active hardware.

| Pin | Owner | Used for |
| --- | --- | --- |
| GP23, GP24, GP25, GP29 | Reserved: kernel | Pico W radio/internal signals; never allocate externally |
| GP0 |Mahesha | Right encoder B, Grove 1, green wire; UART0 console/crash conflict below |
| GP1 |Mahesha | Right encoder A, Grove 1, red wire; UART0 console/crash conflict below |
| GP2 |Jia Jing | Reported HC-SR04P Echo, Grove 2; shares Maker port |
| GP3 |Jia Jing | Reported HC-SR04P Trig, Grove 2; shares Maker port |
| GP4 |Darrius | GY-511 SDA, Grove 3 / I2C0 |
| GP5 |Darrius | GY-511 SCL, Grove 3 / I2C0 |
| GP6 | Unclaimed | Check carrier sharing before allocation |
| GP7 |Mahesha | Left encoder B, Grove 7, green wire |
| GP8 |Mahesha | Left motor M1A, black wire; exclusive motor writer |
| GP9 |Mahesha | Left motor M1B, yellow wire; exclusive motor writer |
| GP10 |Mahesha | Right motor M2A, black wire; exclusive motor writer |
| GP11 |Mahesha | Right motor M2B, yellow wire; exclusive motor writer |
| GP12 |Jia Jing | SG90 Servo 1 signal S, orange wire; current bench holds signal low |
| GP13 | Unclaimed | Check carrier sharing before allocation |
| GP14 | Unclaimed | Check carrier sharing before allocation |
| GP15 | Unclaimed | Check carrier sharing before allocation |
| GP16 |Nicholas | Barcode IR DOUT, Grove 4; AOUT disconnected |
| GP17 | Unclaimed | Grove 4 second signal unused |
| GP18 | Unclaimed | Onboard RGB LEDs; no project firmware owner |
| GP19 | Unclaimed | Check carrier sharing before allocation |
| GP20 |Nicholas | Onboard active-low capture/start/reset; proposed Safety input |
| GP21 |Nicholas | Onboard active-low stop/cancel; independent stop path |
| GP22 |Nicholas | Onboard PWM buzzer; mute switch controls audibility |
| GP26 |Nicholas | Left line IR AOUT, Grove 5 / ADC0; disconnect Grove 6 shared GP26 contact |
| GP27 |Nicholas | Right line IR AOUT, Grove 6 / ADC1 |
| GP28 |Mahesha | Left encoder A, Grove 7, red wire; ADC2-capable but used digitally; open battery jumper |

### Supply and disconnected wires

- Power bank: red → green terminal VIN+, black → VIN−. Verify polarity and follow [power and flashing](GETTING_STARTED.md#power-and-flashing).
- Encoders: white → 3V3, blue → GND. Line/barcode IR: VCC → 3V3, GND → GND; both line DOUT wires stay disconnected.
- GY-511: Vin → 3V3, GND → GND; separate `3.3`, INT1, INT2 and DRDY stay disconnected. Reported HC-SR04P: VCC → 3V3, GND → GND; verify the installed variant and Echo HIGH before connection.
- SG90: red → +, brown → −; verify carrier silkscreen and supply rail.

### Wiring conflicts

- Grove 5 and Grove 6 share GP26. Joining their analogue outputs is unsafe; keep the Grove 6 GP26 contact disconnected.
- GP28 battery sensing must remain disconnected through its jumper while used by the left encoder.
- UART0 console and crash reporting claim GP0/GP1. Use `CONSOLE=usb_cdc CRASH_REPORT=0` with encoders attached.
- Grove 2 and the Maker port share GP2/GP3. Do not attach independent devices to both.
- Disconnect GY-511 wires reused elsewhere before restoring I2C. Its separate `3.3` pin is not established as an additional supply input.
- Line sensing and digital encoder GP28 consume the external ADC-capable pins. Barcode AOUT requires a reviewed allocation change or additional hardware.

## 3. Peripherals

A **subsystem** is a student's folder; a **task** is a thread of execution; a **peripheral** is a hardware block. Keep application drivers (`drv_<name>.c/.h`) inside the owning subsystem, outside `device/`. Enabling a kernel driver in `config/config_device.h` changes shared configuration and needs coordination.

Each driven peripheral has one owning task; other tasks use its interface. PWM slices have separate owners. Coordinate shared block/timebase initialization: never reset PWM or TIMER underneath another owner.

| Peripheral | Owner subsystem | Owning task | Pins |
| --- | --- | --- | --- |
| UART0 | Kernel, optional | Console/crash service; disabled in encoder profile | GP0/GP1 conflict withMahesha |
| UART1 | Unclaimed | — | Unallocated |
| I2C0 |Darrius, proposed | Terrain; inactive in current bench | GP4/GP5 |
| I2C1 | Unclaimed | — | Unallocated |
| SPI0 | Unclaimed | — | Unallocated |
| SPI1 | Unclaimed | — | Unallocated |
| ADC |Nicholas | `ir_task`; bounded paired `drv_ir.c`; native driver disabled (`DEVCNF_USE_ADC=0`) | GP26/GP27; GP28 allocated to encoder |
| PWM slices 4 and 5 |Mahesha | `motion_task`; 20 kHz at the template's 125 MHz clock | GP8–GP11 |
| PWM slice 3 |Nicholas | `ir_task`; asynchronous 2 kHz buzzer chirps | GP22 |
| PWM slice 6 |Jia Jing, proposed | Obstacle servo control; PWM inactive in bench | GP12 |
| Encoder GPIO IRQs |Mahesha, proposed | Motion; bounded ISR capture | GP0/GP1/GP7/GP28 |
| Barcode GPIO IRQ |Nicholas | S3 bounded capture entry; agreed shared dispatcher/encoder adapter pending | GP16 |
| Echo GPIO IRQ |Jia Jing, proposed | Obstacle; bounded ISR capture | GP2 |
| Hardware TIMER / watchdog tick |Nicholas initialization; shared timebase | Enable 1 µs tick from 12 MHz reference without resetting TIMER; kernel WiFi also reads TIMER | Internal |
| Watchdog |Nicholas | `ir_task`; 250 ms; feed only when valid IR and Motion progress both advance and are at most 60 ms old | Internal |
| USB console | Kernel | USB service on core 0 | USB connector |
| WiFi radio, TCP/IP, MQTT | Kernel | `cyw43_utk_task` on core 0 | GP23–GP25, GP29 |

S2 releases PWM reset without resetting the block; each slice owner configures independently. `USE_PTMR=0`; kernel scheduling uses SysTick. IRQ buffers remain separate between owners.

### WiFi and MQTT are special

All CYW43, lwIP and MQTT operations run inside the kernel's `cyw43_utk_task`. Other tasks may read connection state through `cyw43_utk_get_status()`. The current Shaun bridge supplies read-only Nicholas telemetry under its optional mutex; it starts no application task and accepts bounded status-only requests, with no actuation commands. Both snapshot writers and readers use nonblocking access. See [Communications](architecture/COMMUNICATIONS.md) for configuration, schema, publication and failure behavior.

## 4. Task priorities

Smaller numbers mean higher priority. Priorities 1–4 are reserved for the kernel: initial task 1, USB 3 and WiFi 4 run on core 0. `TP_PRC1` means core 0; `TP_PRC2` means core 1; zero means any core. Application tasks use core 1 in SMP builds.

### Current bench

| Subsystem | Priority band | Tasks (name – priority – core) |
| --- | --- | --- |
|Shaun Communications | — | No application task; optional snapshot mutex only |
|Mahesha Motion | 13 | `motion_task` – 13 – core 1; 20 ms or stop wake-up |
|Nicholas Line and Barcode | 12 | `task_s_task` – 12 – core 1; buttons 10 ms, paired ADC 20 ms |
|Nicholas bench coordination | 14 | `line_follow_task` – 14 – core 1; local controls and snapshot-to-Motion requests every 10 ms |
|Darrius Terrain | — | Inactive |
|Jia Jing Obstacle | — | Inactive |

### Proposed full robot

| Subsystem | Priority band | Tasks (name – priority – core) |
| --- | --- | --- |
|Shaun Communications | 20 | Reporting bridge – 20 – core 1; network events / [reporting interval](architecture/COMMUNICATIONS.md#business-logic) |
|Mahesha Motion | 13 | Motion – 13 – core 1; 20 ms / stop |
|Nicholas Line and Barcode / coordination | 12, 14–15 | Safety – 12 – core 1, 10 ms / events; Mission – 14 – core 1, 20 ms / events; Line and Barcode – 15 – core 1, ADC 20 ms / barcode IRQ |
|Darrius Terrain | 16 | Terrain – 16 – core 1; 20 ms fresh-data polling |
|Jia Jing Obstacle | 17 | Obstacle – 17 – core 1; bounded acquisition states |

The split and shared configuration are proposals; team acceptance is not recorded. Record every new task and agree priority changes before integration. Kernel network priority stays 4 regardless of the application bridge's priority.

## 5. Interfaces between subsystems

The provider creates and owns each shared object. Current objects stay private behind public subsystem APIs; callers do not manipulate their IDs. Agree new data formats and ownership before coding. IF-04 records the internal tracker/coordinator boundary and allocates no kernel object.

| ID | From → To | Mechanism | Object ID name | Data / meaning | Agreed by |
| --- | --- | --- | --- | --- | --- |
| IF-01 |Nicholas → Mahesha | Latest value underMahesha priority-inheritance mutex | `request_mutex` (private); `s2_publish()` | `MotionRequest`: duties, enabled flag and original sample time; [limits and expiry](architecture/IR_MOTION_HANDOFF.md#current-interface-contract) | Team agreement not recorded |
| IF-02 |Nicholas → Mahesha | Persistent event flag + task wake-up, independent of request mutex | `stop_flag`, `motion_tid` (private); `s2_stop()` / `s2_reset_stop()` | Stop latch; fresh local reset invalidates the old request; [ordering limitations](architecture/IR_MOTION_HANDOFF.md#build-and-acceptance) | Team agreement not recorded |
| IF-03 |Mahesha → Nicholas | Coherent health copy underMahesha mutex; stop flag read separately | `request_mutex`, `stop_flag` (private); `s2_health()` | `MotionHealth`: software service progress and latch; no physical brake/stationarity acknowledgement; [contract](architecture/IR_MOTION_HANDOFF.md#current-interface-contract) | Team agreement not recorded |
| IF-04 | IR tracker → Nicholas coordinator | In-task typed value | —; `ir_tracking_output()` | `IrTrackingOutput`: validity, line condition, signed error/steering and sample time; [Line Tracking](architecture/LINE_TRACKING.md#modules-and-public-interface) | Team agreement not recorded |
| IF-05 |Nicholas → Shaun → kernel network service | Latest value under optional Shaun priority-inheritance mutex; `TMO_POL` both sides | `telemetry_mutex` (private); `s1_telemetry_update()` / `s1_telemetry_copy()` | `S1Telemetry`; busy/unavailable access skips diagnostics; [Communications contract](architecture/COMMUNICATIONS.md#modules-and-drivers) | Team agreement not recorded |
| IF-06 | Unclaimed | — | — | — | — |

Full-robot message/queue contracts remain [proposed in ARCHITECTURE](ARCHITECTURE.md#shared-data-rules), without allocated object IDs. Each cross-subsystem interface requires an [integration test record](../testing/integration/TEST_LOG.md); implementation or a saved software result does not establish physical acceptance.

## 6. Kernel object budget

Limits are set in [config/config.h](../config/config.h). The table shows current application allocations, excluding kernel/driver service objects. Limits are total configured capacities, not remaining runtime resources; changes need shared-configuration agreement.

| Object | Limit |Shaun |Mahesha |Nicholas |Darrius |Jia Jing |
| --- | --- | --- | --- | --- | --- | --- |
| Task | 32 | 0 | 1 | 2 | 0 | 0 |
| Semaphore | 16 | 0 | 0 | 0 | 0 | 0 |
| Event flag | 16 | 0 | 1 | 0 | 0 | 0 |
| Mailbox | 12 | 0 | 0 | 0 | 0 | 0 |
| Mutex | 12 | 0 or 1 | 1 | 2 | 0 | 0 |
| Message buffer | 12 | 0 | 0 | 0 | 0 | 0 |
| Fixed memory pool | 12 | 0 | 0 | 0 | 0 | 0 |
| Variable memory pool | 12 | 0 | 0 | 0 | 0 | 0 |
| Cyclic handler | 12 | 0 | 0 | 0 | 0 | 0 |
| Alarm handler | 12 | 0 | 0 | 0 | 0 | 0 |

Shaun attempts one mutex allocation with `MQTT=1`; none with `MQTT=0`. Allocation failure disables optional diagnostics and permits local startup. Mahesha's request and health copies share one mutex; its persistent stop uses one event flag. Nicholas owns two private mutexes: the line snapshot and barcode service/configuration/event FIFO. Barcode uses static 64-edge and eight-event storage, no kernel message buffer or extra task. Both subsystem tasks use the existing 2048-byte stack; barcode stack/timing margins remain unmeasured.

Create all required subsystem objects before starting application tasks. Required initialization/start failure prevents successful startup; optional telemetry failure may skip reporting. Measure stack margin and total object usage, including kernel services, before implementing the proposed task split.
