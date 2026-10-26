# Team, hardware and workflow

Single owner of: members, subsystem ownership, pins, peripherals, hardware facts, task/kernel-object budget, cross-subsystem interfaces, bring-up and workflow. Requirements, data widths and pending decisions: [project-specs](project-specs.md). Shared-data rules and MQTT frame: [architecture](architecture.md).

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

## 3. Peripherals and hardware reference

### 3.1 Peripheral ownership

Application drivers (`drv_<name>.c/.h`) live inside the owning subsystem, outside `device/`. Enabling a kernel driver in `config/config_device.h` changes shared configuration and needs coordination. Each driven peripheral has one owning task; others use its interface. PWM slices have separate owners; never reset PWM or TIMER under another owner. S2 releases PWM reset without resetting the block; each slice owner configures independently. `USE_PTMR=0`; kernel scheduling uses SysTick. IRQ buffers stay separate between owners.

| Peripheral | Owner | Owning task / note | Pins |
| --- | --- | --- | --- |
| UART0 | Kernel, optional | Console/crash service; disabled in encoder profile | GP0/GP1 (conflict with Mahesha) |
| UART1, I2C1, SPI0, SPI1 | Unclaimed | - | Unallocated |
| I2C0 | Darrius, proposed | Terrain; inactive in bench | GP4/GP5 |
| ADC | Nicholas | `ir_task`; bounded paired `drv_ir.c`; native driver disabled (`DEVCNF_USE_ADC=0`) | GP26/GP27 (GP28 is encoder) |
| PWM slices 4, 5 | Mahesha | `motion_task`; 20 kHz at 125 MHz clock | GP8-GP11 |
| PWM slice 3 | Nicholas | `ir_task`; async 2 kHz buzzer chirps | GP22 |
| PWM slice 6 | Jia Jing, proposed | Obstacle servo; inactive in bench | GP12 |
| Encoder GPIO IRQs | Mahesha, proposed | Motion; bounded ISR capture | GP0/GP1/GP7/GP28 |
| Barcode GPIO IRQ | Nicholas | S3 bounded capture entry; shared dispatcher/encoder adapter pending | GP16 |
| Echo GPIO IRQ | Jia Jing, proposed | Obstacle; bounded ISR capture | GP2 |
| Hardware TIMER / tick | Nicholas init; shared timebase | 1 us tick from 12 MHz reference without resetting TIMER; kernel WiFi also reads TIMER | Internal |
| Watchdog | Nicholas | `ir_task`; 250 ms; fed only when valid IR and Motion progress both advance and are at most 60 ms old | Internal |
| USB console | Kernel | USB service, core 0 | USB |
| WiFi, TCP/IP, MQTT | Kernel | `cyw43_utk_task`, core 0 | GP23-GP25, GP29 |

**WiFi/MQTT:** all CYW43, lwIP and MQTT operations run in the kernel's `cyw43_utk_task`; other tasks read state via `cyw43_utk_get_status()`. The Shaun bridge supplies read-only Nicholas telemetry under its optional mutex, starts no application task, accepts bounded status-only requests, and has no actuation commands. Snapshot writers/readers use nonblocking access. See [subsystem1](architecture/subsystem1.md).

### 3.2 Raspberry Pi Pico W / WH (SDK board `pico_w`)

| Item | Specification / use |
| --- | --- |
| Processor | RP2040 dual Cortex-M0+, up to 133 MHz; template uses 125 MHz |
| Memory | 264 KB SRAM, 2 MB flash |
| Interfaces | 26 GPIO; 2 UART, 2 SPI, 2 I2C; 8 PWM slices / 16 channels; 2 PIO / 8 SM; USB 1.1 |
| ADC | 12-bit, max 500 ksample/s; 3 external analogue pins + 1 internal; reference/supply noise affects readings |
| Radio | CYW43439, 2.4 GHz 802.11n WiFi, Bluetooth 5.2 hardware (project uses WiFi only) |
| Size / environment | 51 x 21 x 1 mm; ambient -20 to +70 C; keep metal away from ~14 x 9 mm antenna area |
| Programming | ROM BOOTSEL UF2 or SWD; wireless LED on `WL_GPIO0` (not an RP2040 GPIO) |

| Signal / header pin | Limit / purpose |
| --- | --- |
| VBUS / 40 | USB 5 V +/-10% |
| VSYS / 39 | 1.8-5.5 V board input (not the Robo carrier VIN rating) |
| 3V3 OUT / 36 | Regulated; recommended external load below 300 mA |
| 3V3_EN / 37 | Regulator enable |
| ADC_VREF / 35, AGND / 33 | ADC reference / analogue ground |
| RUN / 30 | Active-low reset |
| GPIO / ADC inputs | 3.3 V domain; never apply 5 V |

ADC_VREF is filtered from 3V3 via ~201 ohm / 2.2 uF; loading can give ~30 mV offset. Averaging improves repeatability, not absolute accuracy; no precision reference is configured. Radio/internal signals are reserved; do not sample internal supply signals in ways that disrupt the radio.

| RP2040 block | Registers / constraints |
| --- | --- |
| ADC | `CS` (channel/enable/start/ready/error), `RESULT` 12-bit, optional `FCS`/FIFO. A completed two-channel read carries one coherent acquisition time. |
| PWM | Per-slice `CSR`, `DIV`, `CTR`, `CC`, `TOP`; f = sys_clk / (divider x (TOP + 1)) free-running. Preserve other slices and shared reset ownership. |
| SIO / GPIO | Input/output, atomic SET/CLR; configure function/pulls/direction explicitly; bounded edge handlers timestamp transitions. |
| TIMER / watchdog | Keep the monotonic timebase; wrap-safe elapsed subtraction; configure watchdog tick without resetting a service-used timer. |

### 3.3 Cytron Robo Pico carrier

| Item | Rating / behavior |
| --- | --- |
| VIN | 3.6-6 V |
| Grove 3V3 rail | 300 mA max total external load; budget all devices together |
| Motor channels | 1 A continuous per channel; 1.5 A peak < 5 s; PWM up to 20 kHz |
| Analogue input | 0-3.3 V |
| Digital input | LOW -0.3-0.8 V, HIGH 2.0-3.6 V (not 5 V tolerant) |
| Digital output | LOW 0-0.5 V, HIGH 2.6-3.3 V under specified conditions |
| Motor/servo supply | VIN-only: ~VIN; USB-only: ~VUSB - 0.4 V; measure under load |

| Input A | Input B | Driver response |
| --- | --- | --- |
| LOW | LOW | Brake |
| HIGH | LOW | Forward electrical direction |
| LOW | HIGH | Reverse electrical direction |
| HIGH | HIGH | High-impedance / coast |

Physical forward depends on motor wiring/installation. Brake is an electrical request, not proof of stationarity. Check wiring with power off; share grounds; keep motor-current returns off sensor ground wiring. The carrier has seven Grove ports, motor terminals, four servo ports, local buttons, buzzer/mute switch, RGB LEDs and a battery-measurement jumper (`VBAT / 2` when closed; VBAT follows the highest relevant supply). Carrier switch cuts power; reset restarts the Pico; motor-test buttons drive motors at full speed independently of the application request path, so keep wheels clear and avoid them when validating unknown wiring. Maker Pi Pico is a separate carrier without Robo motor-driver assumptions.

### 3.4 Motors and encoders

Car connector PH2.0, six pins: 1 encoder GND, 2 encoder A, 3 encoder B, 4 encoder supply, 5 motor +, 6 motor -. The connector drawing does not establish motor voltage/current, stall current, gear ratio, encoder output, pulses/rev or car geometry; do not infer from colors. Measurements go in [project-specs](project-specs.md#pending-decisions).

With decoded counts per wheel revolution `N`, wheel diameter `D`, interval `dt`, track width `b`:

- Travel: `ds = pi * D * delta_count / N`
- Speed: `v = ds / dt`
- Yaw change: `dtheta = (ds_right - ds_left) / b` rad (convert to degrees for the public contract)

`N` includes edge decoding and gearing (raw PPR cannot be substituted without conversion). Direction from quadrature phase. Account for wrap, bounce/noise, missed edges, slip and finite windows. PID/actions: [subsystem2](architecture/subsystem2.md).

### 3.5 TCRT5000-based IR modules (line and barcode)

IR LED illuminates the surface; phototransistor senses reflection. AOUT = analogue reflectance; DOUT = thresholded comparator. Output polarity and board supply rating do not follow from the bare sensor.

| Bare TCRT5000 | Specification |
| --- | --- |
| Package | ~10.2 x 5.8 x 7 mm; visible-light suppression |
| Optical | ~940-950 nm; response peaks ~2.5 mm |
| Relative range | ~0.2-15 mm at >20% relative response (test conditions; not guaranteed module range) |
| LED | Typ. Vf 1.25 V at 60 mA characterization; 60 mA is an absolute max, not a chosen drive; reverse max 5 V |
| Phototransistor | Vce max 70 V, Ic max 100 mA (no licence for high voltage on Pico inputs) |
| Other | Dissipation 200 mW; ambient -25 to +85 C |

Keep LED current within a justified resistor budget. Height, surface, lighting, alignment and board circuitry set contrast. The ST188-based module's figures do not describe these boards.

If the board uses an LM393 (dual comparator, **open-collector** outputs; pull-up rail sets DOUT HIGH; verify AOUT and DOUT levels at the actual supply): supply 2-36 V; common-mode ground to ~VCC - 1.5 V (25 C) / VCC - 2 V (over temperature); ~0.4 mA at 5 V; output saturation typ. 150 mV, max 400 mV at 4 mA/25 C; response ~1.3 us; pinout 1 OUT A, 2 IN- A, 3 IN+ A, 4 GND, 5 IN+ B, 6 IN- B, 7 OUT B, 8 VCC. Comparator trim is separate from analogue black/white calibration. Do not connect unused AOUT/DOUT contacts merely because present. Processing logic: [subsystem3](architecture/subsystem3.md).

### 3.6 Ultrasonic sensor (reported HC-SR04P)

Representative P module: 3-5.5 V, 2-400 cm at 3.3 V; not proof of the installed board. Confirm marking, supply rating and Echo HIGH <= 3.3 V before direct connection; a 5 V-only HC-SR04 needs suitable power and Echo level protection. Standard HC-SR04 reference: 5 V, ~15 mA, 2 cm-4 m, ~3 mm accuracy, ~15 deg beam, eight-cycle 40 kHz burst after trigger >= 10 us (not guaranteed for the P board).

`distance = sound_speed * echo_seconds / 2`; room temperature `distance_cm ~ echo_us / 58`. Temperature and target angle/material/size affect results. Begin repeat-rate planning conservatively above 60 ms, then validate; bound echo waits; distinguish timeout/unknown from clear space. A scan samples reflected surfaces, not exact shape. See [subsystem5](architecture/subsystem5.md).

### 3.7 SG90 servo

Reported: 9 g SG90. A common variant: 4.8-5.0 V, ~1.8 kg.cm stall torque, 0.12 s/60 deg, nominal 180 deg, ~23 x 12.2 x 29 mm; apply only after matching the actual variant; stall current not established. PWM pulse commands position with no angle feedback. Nominal start: 50 Hz, centre 1500 us, span 1000-2000 us. Calibrate endpoints/centre/settling without forcing mechanical stops. Removing pulses is not a brake. Measure the carrier servo rail under load.

### 3.8 GY-511 / LSM303DLHC IMU

Three-axis accelerometer + three-axis magnetometer + temperature; **no gyroscope** (turn rate must be encoder-derived or otherwise justified). 16-bit advertising describes register words; accelerometer effective resolution depends on mode.

- Breakout advertises 3-5 V input; header order `DRDY, INT2, INT1, SDA, SCL, GND, 3.3, Vin` (verify orientation). Regulator/pull-ups not established by label; use four-wire Vin/GND/SDA/SCL; verify operation and idle bus level; leave `3.3` alone.
- Chip VDD 2.16-3.6 V; VDD_IO 1.71 V to ~VDD + 0.1 V; abs max 4.8 V (not a 5 V rating); -40 to +85 C; ~110 uA is mode/rate-specific.
- I2C 100/400 kHz. Seven-bit addresses: accelerometer `0x19`, magnetometer `0x1E` (bus bytes `0x32/0x33`, `0x3C/0x3D`).
- Pull-ups required (10 kohm reference); check breakout pull-ups/capacitance first. Keep high current and magnetic objects away.
- Poll data-ready or use validated interrupt wiring; cached repeat reads are not fresh. Never write reserved/factory registers.

| Accel register | Purpose |
| --- | --- |
| `CTRL_REG1_A 0x20` | Rate, low-power, axes; reset `0x07` enables axes but stays powered down |
| `CTRL_REG4_A 0x23` | Block-data update, full scale, high resolution |
| `STATUS_REG_A 0x27` | Data-ready / overrun |
| `OUT_X_L_A`-`OUT_Z_H_A 0x28-0x2D` | XYZ low then high; combine signed words |
| `FIFO_CTRL/STATUS 0x2E/0x2F` | Optional FIFO |
| Interrupt / click groups | Optional events; thresholds need calibrated config |

Burst reads need the subaddress auto-increment bit. High-resolution sample is signed left-aligned **12-bit** (arithmetic shift right by 4). Rates 1, 10, 25, 50, 100, 200, 400 Hz (higher mode-dependent).

| Accel full scale | High-res sensitivity |
| --- | --- |
| +/-2 g | 1 mg/LSB |
| +/-4 g | 2 mg/LSB |
| +/-8 g | 4 mg/LSB |
| +/-16 g | 12 mg/LSB |

| Mag register | Purpose |
| --- | --- |
| `CRA_REG_M 0x00` | Rate / temperature enable |
| `CRB_REG_M 0x01` | Gain |
| `MR_REG_M 0x02` | Reset `0x03` sleep; `0x00` continuous |
| Outputs `0x03-0x08` | Order **X, Z, Y** (high/low); reorder to XYZ |
| `SR_REG_M 0x09` | Ready / lock |
| ID `0x0A-0x0C` | Expect `0x48, 0x34, 0x33` |
| Temp `0x31/0x32` | Signed left-aligned 12-bit, 8 LSB/C; not calibrated absolute |

Mag rates 0.75, 1.5, 3, 7.5, 15, 30, 75, 220 Hz. Reject the `-4096` overflow code.

| Mag full scale (gauss) | XY (LSB/gauss) | Z (LSB/gauss) |
| --- | ---: | ---: |
| +/-1.3 | 1100 | 980 |
| +/-1.9 | 855 | 760 |
| +/-2.5 | 670 | 600 |
| +/-4.0 | 450 | 400 |
| +/-4.7 | 400 | 355 |
| +/-5.6 | 330 | 295 |
| +/-8.1 | 230 | 205 |

Gravity-derived pitch/roll is valid only after mounted-axis/bias calibration and near-1 g acceleration; guard against dynamic acceleration, slip and magnetic disturbance. Classification, height validation, peak retention: [subsystem4](architecture/subsystem4.md).

### 3.9 Barcode samples

Printed samples **A and Z**, each at two scales (larger ~2x). Both have 15 black bars in three groups of five, so bar counts cannot distinguish them. Sequences alternate black/white, starting and ending black; `n` narrow, `w` wide; inter-character gaps narrow. Observed wide:narrow ~3:1 (not a calibrated tolerance).

| Sample | First group | Middle group | Last group |
| --- | --- | --- | --- |
| A | `nwnnwnwnn` | `wnnnnwnnw` | `nwnnwnwnn` |
| Z | `nwnnwnwnn` | `nwwnwnnnn` | `nwnnwnwnn` |

Consistent with Code 39 `*A*` / `*Z*` (inference, not a confirmed spec). Width tolerances, quiet zone, mounting coordinate and junction offset remain pending ([project-specs](project-specs.md#pending-decisions) D01/D02).

## 4. Task priorities and kernel object budget

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

## 5. Interfaces between subsystems

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

## 6. Getting started

Requirements in [project-specs](project-specs.md). Full-robot tasks are in section 6.6.

### 6.1 Install tools

Needs git, make, host C++ compiler, Pico C SDK and `arm-none-eabi-gcc`. Windows: build in WSL Ubuntu.

```sh
xcode-select --install                                    # macOS
sudo apt update && sudo apt install -y git make g++ gcc-arm-none-eabi   # Ubuntu / WSL
```

For macOS/manual compilers use the matching `arm-none-eabi` package from Arm's GNU toolchain downloads (WSL: Linux host package); extract and add `bin` to PATH. Template uses SDK 2.2.0 (2.3.0 also recorded working):

```sh
mkdir -p ~/pico && cd ~/pico
git clone --branch 2.2.0 https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk && git submodule update --init lib/tinyusb lib/cyw43-driver lib/lwip
```

In `~/.zshrc` or `~/.bashrc` (SDK path must have **no spaces**; a symlink works):

```sh
export PATH="$HOME/tools/arm-gnu-toolchain-<version>/bin:$PATH"
export PICO_SDK_PATH="$HOME/pico/pico-sdk"
```

Check: `arm-none-eabi-gcc --version`; `ls "$PICO_SDK_PATH/src"`. Clone (private, access required): `git clone https://github.com/shauniee/Autonomous-Robotic-Car.git`. In the coursework workspace, `code/` is the repository root; run commands there.

### 6.2 Build profiles

Build sequentially (shared `build_make/`). Give every flag each time; incompatible objects rebuild automatically.

```sh
# USB-only dev; motors always inhibited
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=none MQTT=0 CRASH_REPORT=0 TEST=1
# Normal offline bench: same with TEST=0
# Wireless dev (motors inhibited)
make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=1
# Normal wireless bench: same with TEST=0
```

| Profile | Output in `build_make/` |
| --- | --- |
| Offline TEST | `mtk3pico_smp1_usb_cdc_wifi_test.uf2` |
| Offline normal | `mtk3pico_smp1_usb_cdc_wifi.uf2` |
| Wireless TEST | `mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt_test.uf2` |
| Wireless normal | `mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt.uf2` |

| Option | Meaning |
| --- | --- |
| `SMP=1`/`0` | Dual/single-core kernel; project uses dual |
| `CONSOLE=usb_cdc`/`uart` | USB serial / UART0 (uses encoder pins; unsuitable with encoders) |
| `WIFI=cyw43`/`none` | W/WH radio / none (ordinary Pico must use `none`) |
| `NET=none`/`lwip`, `MQTT=0`/`1` | Offline / TCP-IP; MQTT needs lwIP + WiFi |
| `TEST=1` | Firmware self-tests; forces S2 braking even if the controller enables motion |
| `CRASH_REPORT=1` | Crash/console copy on UART0; project uses `0` (GP0/GP1 conflict, section 2) |

`arm-none-eabi-gcc not found`: PATH wrong. Unset SDK path or missing USB/radio/network headers: fix SDK path/submodules. No active CMake target or dashboard build. Ordinary-Pico/Maker development uses the preserved handoff profile in [subsystem3](architecture/subsystem3.md).

### 6.3 Power and flashing

**One power source at a time.** USB alone is for flashing and inhibited desktop checks. Disconnect USB before battery/power-bank operation; never add USB for logs during a powered trial. Reboot clears calibration; recalibrate after changing power.

1. Check pins/conflicts (section 2) and electrical limits (section 3). Current bench: motors, two line sensors, buttons, buzzer; barcode/IMU/sonar not active; servo signal held low.
2. Hold BOOTSEL while connecting USB; release when **RPI-RP2** appears; copy the matching **TEST** UF2 to the drive (WSL: `explorer.exe build_make`). The Pico reboots.
3. Open USB serial at 115200: `screen /dev/tty.usbmodem<device> 115200` (Linux `/dev/ttyACM0`; Windows PuTTY/Serial Monitor on the COM port). Quit screen: Ctrl-A, K.
4. Inspect startup, `[TEST] <ID> PASS|FAIL`, `[IR] ... SENSOR_ONLY` / `[LINE] ... MOTOR_INHIBITED`. Console output reports acquisition/calibration/state, not motor movement or measured duty. Record failures via [testing](../testing/README.md).
5. Before normal physical use: resolve the reported motor problem; record raised-wheel baseline, polarity and stop response. Flash the normal image, disconnect USB, connect the battery/power bank alone. Use camera/scope and optional wireless observations as physical evidence.

### 6.4 Calibration and local controls

Calibration is RAM-only; use the real mounting and surface. Enable the buzzer mute switch for audible feedback.

| Step | Action | Expected response |
| --- | --- | --- |
| Black | Stopped/Ready, release then press GP20 with both sensors over black; hold through capture | One chirp on completed capture |
| White | Release then press GP20 over white; hold through capture | Two chirps valid pair; three rejection |
| Start | Sensors on visible line, release then press GP20 | FOLLOW_LINE if calibration/data qualify; TEST still inhibits motors |
| Stop | Press GP21 | Persistent stop; capture cancelled |
| Reset | Clear cause, release GP21, freshly press GP20 | STOPPED to READY; valid calibration kept |
| Restart | Release GP20, press again on a visible line | Fresh start; held buttons never restart |

Capture duration/contrast and classification: [subsystem3](architecture/subsystem3.md). Starting on fully white or broad black is rejected. Reboot replaces a completed calibration; failed white contrast clears both references. A plausible raw ADC value does not prove correct wiring. Controls are local; USB text commands and MQTT calibration/start/stop are retired. If capture is rejected, keep stopped and check connection, height, lighting, surface, raw contrast. If the controller stops, record the reason before reset. Software state does not prove the wheels are stationary.

### 6.5 Wireless telemetry

Use a reachable private 2.4 GHz LAN; the lab broker allows anonymous access without TLS, so keep it on the trusted bench network.

1. `test -e config/wifi_secrets.h || cp config/wifi_secrets.example.h config/wifi_secrets.h`; edit `WIFI_SSID`, `WIFI_PASSWORD`, `MQTT_BROKER_IP` (laptop LAN IPv4, not `127.0.0.1`/`0.0.0.0`); for authenticated brokers also `MQTT_USERNAME`, `MQTT_PASSWORD`. Client-ID prefix gets a radio-MAC suffix; older local headers may omit new settings. The file is git-ignored; do not commit or print it.
2. macOS: `brew install mosquitto`, `ipconfig getifaddr en0` (use the WiFi interface's address if different), `mosquitto -c config/mosquitto-lab.conf`. Linux/WSL: `mosquitto` + `mosquitto-clients`; ensure Pico reachability across WSL/firewall boundaries.
3. Second terminal: `mosquitto_sub -h 127.0.0.1 -p 1883 -t 'robot/s3/#' -v`
4. Flash wireless TEST on USB alone and confirm messages originate from the Pico. After physical readiness, use normal firmware on battery alone and recalibrate.

Laptop helper (heartbeat freshness, read-only status):

```sh
python3 -m pip install -r scripts/requirements-mqtt.txt
python3 scripts/mqtt_client.py monitor --broker <broker-lan-ip>
python3 scripts/mqtt_client.py status --broker <broker-lan-ip>
```

Optional auth via `ROBOT_MQTT_USERNAME` / `ROBOT_MQTT_PASSWORD`; `--root` / `--port` for other configs. The helper subscribes before requesting status, ignores cached retained status as live, and times out without a live response. Firmware accepts only `{"request":"status"}` on `robot/s3/request`; start/stop/drive/calibration messages are rejected; status requests never change local control. Topic, schema, fields, limits and reconnect: [subsystem1](architecture/subsystem1.md). Use validity/age flags; requested duty is not measured output; `motion_enabled` never overrides TEST inhibition; slow telemetry cannot prove brief recovery or stop timing; broker loss does not change healthy local control.

### 6.6 Bring-up order and project tasks

1. Verify carrier/module, wiring, power, sensor levels; inhibited startup and calibration (All / S3).
2. Diagnose motors; raised-wheel output, polarity, stop/reset limits, encoder counts/geometry (S2 / All).
3. Tune wheel PID; straight/distance/turn accuracy; calibrated P line guidance at mounted height (S2 / S3).
4. Capture known barcode patterns, reject invalid reads, implement one-shot navigation events (S3).
5. Restore IMU wiring; calibrate axes; validate hump detection/height against references (S4).
6. Verify sonar/servo; bounded coarse/fine profiles with independent forward freshness (S5).
7. Integrate proposed Mission/Safety contracts, action leases, scan preconditions, bypass, original-line recovery (All).
8. Integrate final mission telemetry; validate status subscription/recovery on Pico; measure network-load isolation (S1 / All).
9. Record subsystem, integration and full mission acceptance; rehearse demo; confirm instructor access (All).

Unknown values: [project-specs](project-specs.md#pending-decisions).

## 7. Coding rules

Apply to new or changed application code (imported kernel need not be reformatted). Informed by Barr Group 2018 embedded C guidance; not a compliance certification.

- **Language/layout:** GNU C11 (`-std=gnu11`). Use C11 acquire/release for word snapshots and startup publication; no atomic RMW helpers on Cortex-M0+. Isolate other extensions in drivers. Four spaces, LF, one statement per line, braces on every body, opening brace on its own line, ~80 columns. Space operators, parenthesize precedence, comment purpose/units/bounds/assumptions, remove dead code.
- **Modules:** one purpose each; lowercase `snake_case.c/.h`, guarded header, minimal exports; never include a `.c` or absolute path. Public functions prefixed with module name; private `static`. Named constants; prefer functions to macros (macros evaluate args once, parenthesized). Types `module_name_t`.
- **Data:** fixed-width ints where width matters; packed bits for stored/shared flags; `bool` only for locals/returns; `const` for immutable. Consistent unsigned literals; justify narrowing casts; avoid signed bit ops. Initialize before use; explicit invalid codes. `float` only for justified local computation (`f` suffix, finite/range checks); stored/shared values are fixed point with documented scale (widths and scales in [project-specs](project-specs.md)). In-RAM packed state may use bit-fields; wire formats (MQTT) use explicit masks/shifts with documented byte order, never native structs, padding or bit-field order.
- **Minimum-bit:** each field gets `ceil(log2(values))` bits including the invalid code; record range, unit, signedness, scale, rounding, invalid code, overflow behavior in project-specs before converting. Exclusive states use one encoded value; coexisting faults use independent bits. Combine fields in one unit only with a shared writer. Access via `module_field_get()/_set()`; no caller masks. Loop counters/temporaries exempt. Test min, max and invalid code; overflow saturates or faults as documented.
- **Control:** check return codes; safe failure responses; `default` in switches; document fallthrough. Bound polling, waits, retries, buffers, actions; validate counts/indices/inputs; keep acquisition time; wrap-safe elapsed time.
- **Concurrency:** `volatile` is for hardware/async access, not synchronization. Share data only via the three patterns in [architecture](architecture.md): one-word snapshot (single store/load), mutex copy above 32 bits, ISR-to-task ring. One writer per storage unit; only it does RMW. Packed data shared with a core-1 ISR updates inside `DI`/`EI`; packed data written from both cores needs a mutex or RP2040 hardware spinlock (`DI` masks only the local core). ISRs: capture, signal, return; no blocking, formatting, decoding, allocation; use `_task`/`_isr` names.
- **Ownership:** one motor writer, separate peripheral owners, kernel-only networking; Stop must not depend on diagnostics queues or the request mutex. No credentials in version control.

**Review checklist:** purpose/API/units/ownership clear, no duplicate mutable state; style consistent; inputs, casts, arithmetic, finite values, bounds checked; hardware actions bounded, failures inhibit unsafe output; shared values synchronized, cached observations keep acquisition time; stop, stale data and reset keep the contract; no ISR blocking, foreign motor writes, student network calls or committed credentials; packed fields match project-specs, use accessors, have boundary tests; one writer per storage unit; evidence names source/profile (a build is not hardware proof).

## 8. Workflow (issue branch)

One GitHub issue = one branch = one PR = one squash merge into `main`.

1. Start from an open issue; if none, propose title, labels, scope to the user (do not create unasked).
2. Branch from current `main`, named as the issue title `<type>/<slug>` (type: `feat`, `fix`, `chore`, `docs`, `test`; e.g. `docs/docs-cleanup`). Never commit to `main`, `uat` or `legacy-main`.
3. Stay in the issue's Scope; respect Out of scope and Depends on; do not mix issues; if work needs an unmerged issue, stop and report.
4. Small commits, one logical change each, prefix `feat:`, `fix:`, `chore:`, `docs:` or `test:` (optional scope, e.g. `test(s3):`).
5. Verify before handoff with the smallest proof (build profile, link check, test record) and state what was and was not executed. Record the exact committed revision with each test result ([testing](../testing/README.md)).
6. Push/open or update the PR only when asked. The user performs the squash merge; never merge, rebase onto or force-push `main`; never delete the user's branches.
7. Squash message: `<prefix>: <summary> (#<PR>)`, body summarizing net change, `Closes #<issue>`.
8. After merge, branch the next issue from updated `main`; do not reuse merged branches.

Never commit `config/wifi_secrets.h`, credentials, `*.elf`/`*.uf2`/`*.map` or videos (git-ignored).
