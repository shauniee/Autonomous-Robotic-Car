# Team and subsystems

This guide records who owns each subsystem, pin, peripheral, task and kernel object. A subsystem is one student's part of the robot, with its own folder; it may contain several tasks and peripherals.


## 1. Members and subsystems

| # | Subsystem (what it does) | Student ID | GitHub username | Code | Tests |
| --- | --- | --- | --- | --- | --- |
| 1 | **Shaun — Communications:** WiFi/MQTT integration, telemetry and reporting | 2500629 | Not recorded | [app_program/subsystem1/](../app_program/subsystem1) | [testing/subsystem1/](../testing/subsystem1) |
| 2 | **Mahesha — Motion:** motors, encoders, wheel PID and motion APIs | 2503973 | Not recorded | [app_program/subsystem2/](../app_program/subsystem2) | [testing/subsystem2/](../testing/subsystem2) |
| 3 | **Nicholas — Line and Barcode:** IR, barcode, Mission/Safety coordination and integration | 2501552 | Not recorded | [app_program/subsystem3/](../app_program/subsystem3) | [testing/subsystem3/](../testing/subsystem3) |
| 4 | **Darrius — Terrain:** IMU processing, motion classification and hump measurement | 2500360 | Not recorded | [app_program/subsystem4/](../app_program/subsystem4) | [testing/subsystem4/](../testing/subsystem4) |
| 5 | **Jia Jing — Obstacle:** sonar, servo scanning and obstacle profiling | 2502825 | Not recorded | [app_program/subsystem5/](../app_program/subsystem5) | [testing/subsystem5/](../testing/subsystem5) |

## 2. Pins
 [TECHNICAL_INFO](donotsubmit/TECHNICAL_INFO.md) owns electrical limits and connector definitions; [GETTING_STARTED](../GUIDE.md#one-time-preparation) identifies active hardware.

| Pin | Owner | Used for |
| --- | --- | --- |
| GP23, GP24, GP25, GP29 | Reserved: kernel | Pico W radio/internal signals; never allocate externally |
| GP0 | Mahesha | Right encoder B, Grove 1, green wire; UART0 console/crash conflict below |
| GP1 | Mahesha | Right encoder A, Grove 1, red wire; UART0 console/crash conflict below |
| GP2 | Jia Jing | Reported HC-SR04P Echo, Grove 2; shares Maker port |
| GP3 | Jia Jing | Reported HC-SR04P Trig, Grove 2; shares Maker port |
| GP4 | Darrius | GY-511 SDA, Grove 3 / I2C0 |
| GP5 | Darrius | GY-511 SCL, Grove 3 / I2C0 |
| GP6 | Unclaimed | Check carrier sharing before allocation |
| GP7 | Mahesha | Left encoder B, Grove 7, green wire |
| GP8 | Mahesha | Left motor M1A, black wire; exclusive motor writer |
| GP9 | Mahesha | Left motor M1B, yellow wire; exclusive motor writer |
| GP10 | Mahesha | Right motor M2A, black wire; exclusive motor writer |
| GP11 | Mahesha | Right motor M2B, yellow wire; exclusive motor writer |
| GP12 | Jia Jing | SG90 Servo 1 signal S, orange wire; held low until configured |
| GP13 | Unclaimed | Check carrier sharing before allocation |
| GP14 | Unclaimed | Check carrier sharing before allocation |
| GP15 | Unclaimed | Check carrier sharing before allocation |
| GP16 | Nicholas | Barcode IR DOUT, Grove 4; AOUT disconnected |
| GP17 | Unclaimed | Grove 4 second signal unused |
| GP18 | Unclaimed | Onboard RGB LEDs; no project firmware owner |
| GP19 | Unclaimed | Check carrier sharing before allocation |
| GP20 | Nicholas | Onboard active-low capture/start/reset; Safety input |
| GP21 | Nicholas | Onboard active-low stop/cancel; independent stop path |
| GP22 | Nicholas | Onboard PWM buzzer; mute switch controls audibility |
| GP26 | Nicholas | Left line IR AOUT, Grove 5 / ADC0; disconnect Grove 6 shared GP26 contact |
| GP27 | Nicholas | Right line IR AOUT, Grove 6 / ADC1 |
| GP28 | Mahesha | Left encoder A, Grove 7, red wire; ADC2-capable but used digitally; open battery jumper |

### Supply and disconnected wires

- Power bank: red → green terminal VIN+, black → VIN−. Verify polarity and follow [power and flashing](../GUIDE.md#power-and-flashing).
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
| I2C0 | Darrius | Terrain, bounded direct driver | GP4/GP5 |
| I2C1 | Unclaimed | — | Unallocated |
| SPI0 | Unclaimed | — | Unallocated |
| SPI1 | Unclaimed | — | Unallocated |
| ADC | Nicholas | `task_s_task`; bounded paired `drv_ir_tracking.c`; native driver disabled (`DEVCNF_USE_ADC=0`) | GP26/GP27; GP28 allocated to encoder |
| PWM slices 4 and 5 | Mahesha | `motion_task`; 20 kHz at the template's 125 MHz clock | GP8–GP11 |
| PWM slice 3 | Nicholas | `task_s_task`; asynchronous 2 kHz buzzer chirps | GP22 |
| PWM slice 6 | Jia Jing | Obstacle; disabled until calibrated | GP12 |
| Encoder GPIO IRQs | Mahesha | Motion; bounded ISR capture | GP0/GP1/GP7/GP28 |
| Barcode GPIO IRQ | Nicholas | S3 bounded capture entry; shared dispatcher and direct encoder adapter | GP16 |
| Echo GPIO IRQ | Jia Jing | Obstacle; bounded ISR capture | GP2 |
| Hardware TIMER / watchdog tick | Shared timebase | Enable 1 µs tick from 12 MHz reference without resetting TIMER; kernel WiFi also reads TIMER | Internal |
| Watchdog | Shared | Safety; 250 ms normal-image supervision of local service progress; TEST disabled | Internal |
| USB console | Kernel | USB service on core 0 | USB connector |
| WiFi radio, TCP/IP, MQTT | Kernel | `cyw43_utk_task` on core 0 | GP23–GP25, GP29 |

Mahesha releases PWM reset without resetting the block; each slice owner configures independently. `USE_PTMR=0`; kernel scheduling uses SysTick. IRQ buffers remain separate between owners.

### WiFi and MQTT are special

All CYW43, lwIP and MQTT operations run inside the kernel's `cyw43_utk_task`. Other tasks may read connection state through `cyw43_utk_get_status()`. The current S1 reporting task supplies read-only subsystem/Mission snapshots under its optional mutex. Kernel subscription supports bounded status and stopped-only mapping configuration, with no actuation commands. Snapshot access is nonblocking. See [Communications](architecture/SUBSYSTEM1.md) for configuration, schema, publication and failure behavior.

## 4. Task priorities

Smaller numbers mean higher priority. Priorities 1–4 are reserved for the kernel: initial task 1, USB 3 and WiFi 4 run on core 0. `TP_PRC1` means core 0; `TP_PRC2` means core 1; zero means any core. Application tasks use core 1 in SMP builds.

### UAT tasks

| Owner | Task / priority / core | Release |
| --- | --- | --- |
| Nicholas | Safety / 12 / core 1 | 10 ms |
| Mahesha | Motion / 13 / core 1 | 20 ms or stop wake-up |
| Nicholas | Mission / 14 / core 1 | 20 ms |
| Nicholas | Line/Barcode / 15 / core 1 | Inputs 10 ms, ADC 20 ms, bounded edge budget |
| Darrius | Terrain / 16 / core 1 | 20 ms, fresh-data gated |
| Jia Jing | Obstacle / 17 / core 1 | Bounded acquisition states, 2 ms service |
| Shaun | Reporting / 20 / core 1 | Nominal 100 ms, optional with MQTT |

Application stacks are 4096 bytes. TEST startup retains its separate 4096-byte stack; normal startup is 1024 bytes. Scheduling, stack margin and stop latency require physical measurement; configured periods are not recorded WCET.

## 5. Interfaces between subsystems

Providers own private objects and public snapshot APIs. See [Architecture](ARCHITECTURE.md#shared-contracts) for current data flow and [Motion](architecture/SUBSYSTEM2.md#control-and-contracts) for command/feedback semantics. Legacy IF-01 duty publication is retired; IF-02 stop/reset, IF-03 health, IF-04 IR guidance and IF-05 reporting remain recognizable boundaries. Encoder actions and measured feedback extend them; no direct foreign PWM access is allowed.

Nicholas's Mission is the sole command producer. Core-0 configuration is a bounded copied handoff, revalidated by Mission on core 1. Shared GPIO dispatch services encoder pins before GP16 and acknowledges owned bits only; the USB SDK shim is not a general dispatcher.

All bounded polling mutexes, including S1 reporting and Mission/network handoff, use priority ceiling 1. This permits startup/core-0 readers and prevents a higher-priority polling reader from preempting a writer holding the same lock on core 1. Hold times cover bounded copies/processing only: no waits, I2C or logging. Base task priorities above are unchanged; cross-core copies can still return busy. Optional reporting reads remain nonblocking; Mission state logging occurs after releasing its snapshot lock. INT-25 checks the actual kernel ceiling/restore behavior; T15 must measure worst-case hold times and stop latency.

## 6. Kernel object budget

| Object | Application use | Configured limit |
| --- | --- | --- |
| Tasks | 6 required + 1 optional reporting | 32 total, including kernel services |
| Mutexes | S1 0/1, S2 1, S3 3, S4 1, S5 1: total 6/7 | 12 total |
| Event flags | S2 persistent stop: 1 | 16 total |
| Queues / buffers | Static barcode ring 64 and copied encounter FIFO 8; one mapping handoff and one scan request | Fixed application capacities |

No application mailbox or kernel message buffer is allocated. Required objects are created before task start; optional reporting failure is tolerated. Measure runtime memory and stack margins under concurrent load. [Testing](../testing/README.md) owns evidence.
