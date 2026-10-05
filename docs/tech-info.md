# Autonomous robotic car — technical reference

## Authority and current setup

The [design review](<../../CSC01 Design Review.docx>) controls the integrated design. Datasheet facts below retain their source provenance; the Word file does not override the physical ADC mapping or electrical limits. The review lists right IR GP26 and left IR GP27, but the current user-requested bench swaps them: right GP27/ADC1, left GP26/ADC0. GP27 is ADC1 despite the review’s ADC0 label. See [hardware](HARDWARE.md) and [audit](DESIGN_REVIEW_AUDIT.md).

## How to read this file

This file is the **standalone working reference for all 14 PDFs** in `document/`, including the two assignment PDFs. It records the project requirements and the hardware facts needed for design, wiring, firmware, calibration, and testing so that normal project work can start here without reopening a PDF. Manufacturer-specific details not stated in the supplied documents remain explicitly unknown. Page numbers below are included for provenance, not as a substitute for the information in this file. [project-info.md](project-info.md) remains the fuller assignment-focused companion.

**Current evidence boundary:** Nicholas confirmed a **Pico WH on a Robo Pico carrier**, a **9 g SG90** scanning servo, and an **HC-SR04P** ultrasonic module, and reported that the power bank's red and black leads go to the carrier's green `+` and `−` input terminal. The exact ultrasonic board marking and its measured Echo HIGH level have not been independently checked. The other installed sensor-board variants, encoder supply voltage, measured power-bank voltage/current, and existing signal wires also need verification. The consolidated intended connection map is in [architecture.md](ARCHITECTURE.md); older pin suggestions in the supplied references are not that map.

### Source inventory and what each PDF contributes

| PDF | Pages | Information captured below |
| --- | ---: | --- |
| [Project Briefing.pdf](<../../document/Project Briefing.pdf>) | 10 | Integrated mission, five-buddy responsibilities and deliverables, assessment themes, Week 10 demonstration advice. |
| [Project Write-up.pdf](<../../document/Project Write-up.pdf>) | 11 | Ten mission outcomes, expected behavior, suggested architecture, buddy tasks, example scans/telemetry, assessment. |
| [RP-008312-DS-2-pico-w-datasheet.pdf](../../document/RP-008312-DS-2-pico-w-datasheet.pdf) | 26 | Pico W/WH resources, full 40-pin map, 3.3 V GPIO, ADC, power and antenna constraints, UF2/SWD. |
| [Robo_Pico.pdf](../../document/Robo_Pico.pdf) | 10 | Carrier power, Grove/Maker/servo/motor pin assignments, shared pins, electrical ratings and motor truth table. |
| [PicoCar.pdf](../../document/PicoCar.pdf) | 4 | Illustrated six-wire motor/encoder, GY-511 breakout header, generic Pico pinout and Robo Pico board layout. |
| [Infrared-Reflective-Sensor-UserManual.pdf](../../document/Infrared-Reflective-Sensor-UserManual.pdf) | 2 | Waveshare ST188/LM393 module voltage, dimensions, four-pin interface and example behavior. |
| [tcrt5000.pdf](../../document/tcrt5000.pdf) | 7 | Bare Vishay reflective LED/phototransistor limits, optical working distance and package. |
| [LM393-D.PDF](../../document/LM393-D.PDF) | 13 | Dual comparator behavior, supply/input/output limits, package pinout and hysteresis considerations. |
| [Barcode & Line Specification.pdf](<../../document/Barcode & Line Specification.pdf>) | 1 | Photographed line width, line-to-barcode gap and barcode length. |
| [Barcode Sample.pdf](<../../document/Barcode Sample.pdf>) | 4 | Actual A and Z visual samples at two scales; bar/space width patterns transcribed below. |
| [OKY3231-2.pdf](../../document/OKY3231-2.pdf) | 1 | GY-511 breakout product description and advertised supply/features. |
| [lsm303dlhc.pdf](../../document/lsm303dlhc.pdf) | 42 | Accelerometer/magnetometer chip limits, I²C addresses, control/data registers, rates, sensitivity and interrupts. |
| [HCSR04.pdf](../../document/HCSR04.pdf) | 3 | **Older HC-SR04**, 5 V-only reference: pin order and basic pulse-ranging method. Its supply rating does not describe the team's HC-SR04P. |
| [barr_c_coding_standard_2018.pdf](../../document/barr_c_coding_standard_2018.pdf) | 87 | Embedded C source organization, types, functions, interrupts, comments and control-flow rules. |

## Assignment facts from the two project PDFs

The car must **autonomously** traverse a predefined course containing a black line, directional barcodes, humps of varying height and static obstacles. The ten required outcomes in the write-up are: (1) follow the line; (2) detect/decode barcodes; (3) perform left, right, straight and U-turn commands; (4) detect humps; (5) estimate and report the highest hump peak of the run; (6) detect obstacles; (7) profile shape and position with sonar; (8) bypass without collision; (9) reacquire the original line; and (10) report telemetry over WiFi. The line algorithm should tolerate modest lighting, track and speed variation. [Write-up pp. 1–3]

The write-up's **example** mapping is A=left, B=right, C=straight, D=U-turn; this is not proof of the final printed symbols or barcode encoding. Its Buddy 3 section lists only left/right/straight, while the system-level requirement includes U-turn. Keep U-turn in the plan unless clarified. The obstacle sequence is slow/stop → coarse/fine scan → estimate location, width, closest point and side clearances → choose continue/left/right/reverse → bypass → find the original line → resume. Example scan angles are 30°, 60°, 90°, 120°, 150°, not mandatory settings. [Write-up pp. 2–3, 7, 9–10]

The suggested architecture has a sensor layer (IR, encoder, IMU, ultrasonic and barcode sensing), control layer (motion, line tracking, navigation and avoidance) and communication layer (WiFi/MQTT/telemetry). A central vehicle controller coordinates the five buddy subsystems. [Write-up p. 4; Briefing p. 3]

| Buddy | Scope and named outputs | Briefing/write-up location |
| --- | --- | --- |
| 1 — communications | WiFi and MQTT connection; topics and payloads; telemetry publishing; command subscription; heartbeat/recovery. Example fields: robot state, speeds/counts/distance, line, barcode, hump and obstacle data. Deliver communication API, framework, MQTT documentation and demo. | Briefing p. 4; Write-up p. 5 |
| 2 — motion | Motor/encoder integration, speed/distance estimate, PID speed control, straight correction, calibrated encoder turns. Example API: forward/backward by distance, left/right by angle, stop. Deliver motion library, tuning report and accuracy evaluation. | Briefing p. 5; Write-up p. 6 |
| 3 — line/barcode | Three IR sensors; calibrate, track line, detect junction, decode barcode, issue navigation events. Deliver line module, decoder and command interface. | Briefing p. 6; Write-up p. 7 |
| 4 — terrain | IMU calibration, tilt/hump/peak height, motion/collision classification, turn monitoring/events. Deliver processing module, hump algorithm and terrain report. | Briefing p. 7; Write-up p. 8 |
| 5 — obstacles | Servo sonar coarse/fine scan, profile/clearance, stop/turn/reverse proposal, bypass and line recovery. Deliver scanner, profile generator, avoidance and recovery algorithms. | Briefing p. 8; Write-up pp. 9–10 |

Every teammate also participates in integration, testing, debugging and demonstration. Assessment covers mission completion, navigation accuracy/time, barcode and hump perception, obstacle avoidance, robustness, software quality/resource efficiency, integration and demonstration evidence. The briefing calls for stable interfaces, repeated subsystem tests, logged calibration/measurements and rehearsal for **Week 10**; neither PDF supplies numerical marks or precise acceptance thresholds. [Briefing pp. 9–10; Write-up pp. 1, 5, 11]

**Current team/course information supplied after the PDFs:** Pico WH in Robo Pico; one straight, one curve and one T-junction; A/Z examples are relevant but final command symbols remain unknown; obstacle and bypass geometry and finish condition are unspecified; task ownership is assigned in the design review (Nicholas integration/Safety/Mission/line, Mahesha motion, Darrius terrain, Jia Jing obstacle, Shaun communications); horizon is about one month; no remote start/stop; hump-height reference method undecided. Treat this paragraph as team input, not a claim from the PDFs.

## Controller: Raspberry Pi Pico W and Robo Pico carrier

### Pico WH microcontroller board

The [Pico W datasheet](../../document/RP-008312-DS-2-pico-w-datasheet.pdf) describes an RP2040 board with dual Cortex-M0+ cores (up to 133 MHz), 264 kB SRAM, 2 MB onboard flash, 26 header-accessible GPIOs, three exposed ADC-capable GPIOs (GP26–GP28), PWM, I²C, SPI, UART, USB, and onboard 2.4 GHz WiFi through a CYW43439. GPIO is fixed at **3.3 V**; the board's exposed pins must not be treated as 5 V logic. [Pico W pp. 4–5, 8, 14]

- `VBUS` (physical pin 40) is USB's nominal 5 V input; `VSYS` (pin 39) feeds the onboard 3.3 V regulator and is specified for 1.8–5.5 V; `3V3` (pin 36) is a regulated output. The datasheet recommends keeping external load on `3V3` below 300 mA, subject to board load and input voltage. `ADC_VREF` and `AGND` have analogue roles; `RUN` resets the chip when pulled low. [Pico W pp. 9, 13–15]
- GP0–GP22 and GP26–GP28 are available as header signals. GP23–GP25 and GP29 are involved in internal Pico W functions; do not allocate them as ordinary carrier wiring. GP26–GP28 may be digital inputs/outputs or ADC inputs, but a pin cannot perform both uses at once. [Pico W pp. 8, 14]
- BOOTSEL plus USB power enters UF2 mass-storage programming mode; copying a built `.uf2` programs flash and restarts the board. WiFi hardware being present does **not** supply an MQTT implementation by itself. [Pico W pp. 5, 14]
- The antenna end needs physical clearance; nearby metal or a battery can reduce radio performance. [Pico W pp. 7, 11]

**Full Pico WH 40-pin header map** (same electrical pin map as Pico W; physical pin number is not the GPIO number): [Pico W pp. 5, 8–9]

| Pin | Signal | Pin | Signal | Pin | Signal | Pin | Signal |
| ---: | --- | ---: | --- | ---: | --- | ---: | --- |
| 1 | GP0 | 11 | GP8 | 21 | GP16 | 31 | GP26 / ADC0 |
| 2 | GP1 | 12 | GP9 | 22 | GP17 | 32 | GP27 / ADC1 |
| 3 | GND | 13 | GND | 23 | GND | 33 | AGND |
| 4 | GP2 | 14 | GP10 | 24 | GP18 | 34 | GP28 / ADC2 |
| 5 | GP3 | 15 | GP11 | 25 | GP19 | 35 | ADC_VREF |
| 6 | GP4 | 16 | GP12 | 26 | GP20 | 36 | 3V3 output |
| 7 | GP5 | 17 | GP13 | 27 | GP21 | 37 | 3V3_EN |
| 8 | GND | 18 | GND | 28 | GND | 38 | GND |
| 9 | GP6 | 19 | GP14 | 29 | GP22 | 39 | VSYS input |
| 10 | GP7 | 20 | GP15 | 30 | RUN / reset | 40 | VBUS / USB 5 V |

**Pico-only electrical and software implications:** there are 26 exposed GPIOs, of which GP0–GP22 are digital-only and GP26–GP28 can also be ADC inputs. The RP2040 silicon has four ADC channels, but GP29/ADC3 is internally used for wireless/SYS voltage handling on Pico W. ADC resolution is 12 bits and the nominal reference follows the filtered 3.3 V supply, so voltage readings need calibration if precision matters. The board's `3V3` output is recommended below 300 mA of **external load**, depending on Pico and supply conditions. Pico W uses `WL_GPIO0` for its onboard LED, not ordinary GP25. GP23–GP25 and GP29 are occupied internally; they are not free header pins. [Pico W pp. 4–5, 8–9, 14]

**Power/programming:** the board expects VBUS 5 V ±10% or VSYS 1.8–5.5 V, but the installed carrier has its own 3.6–6 V input specification. USB BOOTSEL mode accepts a built `.uf2` file; SWD is also exposed. The antenna keep-out is about **14 mm × 9 mm**; avoid mounting metal, batteries or high-current wiring across that end. Board recommended ambient range is approximately −20 to +70 °C. WiFi is onboard through the CYW43439; the Pico W datasheet also lists 2.4 GHz 802.11n and Bluetooth capability, though the assignment only calls for WiFi. [Pico W pp. 4, 6, 11, 13–15, 19]

The installed **Pico WH** is the Pico W variant with headers; use the Pico W electrical pinout and the Robo Pico carrier map. The separate [PicoCar.pdf](../../document/PicoCar.pdf) p. 3 contains a Pico pinout illustration but is less specific about the installed variant.

### Cytron Robo Pico carrier

The [Robo_Pico.pdf](../../document/Robo_Pico.pdf) is the carrier-board datasheet compatible with Pico/Pico W. It contains the motor driver, servo connectors, Grove/Maker connectors, LEDs/buttons/buzzer, power connections, reset, and GPIO breakout. Its board input is **3.6–6 V** through USB, LiPo, or VIN according to its specification table. Nicholas reports the power-bank red/black leads connected to the green VIN `+`/`−` screw terminal; this must be confirmed with a meter for polarity and voltage, and the power bank's available current must cover motors and servo. Motor and servo supply follow the carrier's input/power path, **not** the Pico's 3.3 V logic rail. [Robo Pico pp. 3–8]

| Carrier function | Fixed signal/limit | Source |
| --- | --- | --- |
| Motor 1 H-bridge inputs | GP8 (`M1A`), GP9 (`M1B`) | [Robo Pico p. 5](../../document/Robo_Pico.pdf) |
| Motor 2 H-bridge inputs | GP10 (`M2A`), GP11 (`M2B`) | [Robo Pico p. 5](../../document/Robo_Pico.pdf) |
| Motor driver truth table | low/low = brake; high/low and low/high = opposite directions; high/high = coast. Actual forward polarity depends on motor wires. | [Robo Pico p. 9](../../document/Robo_Pico.pdf) |
| Motor electrical limit | Maximum **1 A continuous** or **1.5 A peak for under 5 s per channel**; driver PWM frequency up to 20 kHz. Compare this with each actual motor's running and stall current. | [Robo Pico p. 7](../../document/Robo_Pico.pdf) |
| Servo signal ports | GP12, GP13, GP14, GP15; servo `V+` follows the carrier power source. Servo voltage/current requirements are absent from the supplied PDFs. | [Robo Pico pp. 4, 7](../../document/Robo_Pico.pdf) |
| Maker/Qwiic-style port | Shares GP2 SDA1 and GP3 SCL1 with Grove port 2; both connectors cannot independently own those pins. | [Robo Pico p. 4](../../document/Robo_Pico.pdf) |
| Onboard peripherals | RGB LED GP18; buttons GP20/GP21; buzzer GP22. Do not assign these without deciding to give up the onboard function. | [Robo Pico p. 4](../../document/Robo_Pico.pdf) |
| Optional battery sense | A **solder jumper** connects VBAT/2 to GP28 ADC2; keep it open because GP28 is the left encoder A input in the current plan. | [Robo Pico p. 5](../../document/Robo_Pico.pdf) |
| 3.3 V peripheral budget | The carrier lists **300 mA total** for Grove 3V3 outputs. Keep sensor loads within that shared limit and assess Pico W WiFi load separately in the overall budget. | [Robo Pico p. 7](../../document/Robo_Pico.pdf) |

The Grove pairs are GP0/GP1 (port 1), GP2/GP3 (port 2), GP4/GP5 (port 3), GP16/GP17 (port 4), GP6/GP26 (port 5), GP26/GP27 (port 6), and GP7/GP28 (port 7). **GP26 appears on both ports 5 and 6**; choosing those sockets does not create two independent GPIOs. The breakout header also exposes the Pico pins. [Robo Pico p. 4]

| Grove port | Two signal GPIOs | Relevant use/conflict |
| ---: | --- | --- |
| 1 | GP0, GP1 | Right encoder B on GP0, A on GP1; no motor drive current. |
| 2 | GP2, GP3 | HC-SR04P Echo on GP2, Trig on GP3; shared with Maker port. |
| 3 | GP4, GP5 | GY-511 I²C0 SDA/SCL. |
| 4 | GP16, GP17 | Barcode IR DOUT on GP16; GP17 unused. |
| 5 | GP6, GP26 | Left line IR AOUT on GP26/ADC0; GP6 unused. |
| 6 | GP26, GP27 | Right line IR AOUT on GP27/ADC1; shared GP26 contact unused. |
| 7 | GP7, GP28 | Left encoder B on GP7, A on GP28; battery-sense jumper open. |

The carrier's digital-input LOW band is −0.3–0.8 V and HIGH band is 2.0–3.6 V; analogue input is 0–3.3 V. Its digital outputs are listed as LOW 0–0.5 V and HIGH 2.6–3.3 V. These carrier figures still do not make a 5 V sensor output safe for the Pico. The motor PWM frequency ceiling is 20 kHz, motor current limit is 1 A continuous / 1.5 A peak under 5 seconds **per channel**, and total Grove 3V3 current is 300 mA. [Robo Pico p. 7]

**Carrier motor truth table** (A is GP8 for M1 or GP10 for M2; B is GP9 for M1 or GP11 for M2): [Robo Pico p. 9]

| A | B | Driver outputs | Named motion |
| --- | --- | --- | --- |
| LOW | LOW | Both LOW | Brake |
| HIGH | LOW | A HIGH, B LOW | Forward, subject to actual wire polarity |
| LOW | HIGH | A LOW, B HIGH | Backward, subject to actual wire polarity |
| HIGH | HIGH | Both high-impedance | Coast |

**Carrier power path:** VIN alone drives Vmotor and Vservo at approximately VIN; USB alone gives them approximately VUSB − 0.4 V. When USB and VIN are present together, which source reaches the motors/servos depends on their relative voltage and the carrier power-path table. The carrier datasheet discourages using LiPo and VIN together. Its on/off switch cuts board power, reset restarts the Pico, motor-test buttons run a motor at full speed, and the optional VBAT-sense solder jumper reports half of the highest relevant supply on GP28/ADC2. Do not press a motor-test button with the wheels restrained or while checking an unknown wire. [Robo Pico pp. 3–5, 7–9]

**Power precautions for the design:** determine whether the power bank feeds the carrier via its USB/Pico USB path or VIN; record the actual rail voltage under motor start and servo stall; keep all signal grounds common; never attach a 5 V output directly to a Pico GPIO; and use the carrier motor terminals instead of driving a motor from a GPIO. These are engineering consequences of the stated voltage/current limits, not a tested wiring layout.

## Geared motors and wheel encoders

[PicoCar.pdf p. 1](../../document/PicoCar.pdf) shows a six-position **PH2.0-6P** motor/encoder interface: `1` encoder GND, `2` encoder A, `3` encoder B, `4` encoder supply, `5` motor +, `6` motor −. The A/B channels suggest quadrature counting and direction detection. The motor leads go to Robo Pico M1/M2 terminals; the encoder supply and A/B signals go to a suitable rail and GPIO inputs **only after their voltage levels are measured or specified**.

| Six-pin motor connector position | Printed function | Destination in current plan |
| ---: | --- | --- |
| 1 | Encoder ground | Common carrier GND |
| 2 | Encoder A phase | Left GP28 or right GP1; verify physical connector labels/wire colors. |
| 3 | Encoder B phase | Left GP7 or right GP0; verify physical connector labels/wire colors. |
| 4 | Encoder power | Grove 3V3 in the current plan; verify module rating and output level. |
| 5 | Motor line + | Left M1A or right M2A in the current plan. |
| 6 | Motor line − | Left M1B or right M2B in the current plan. |

The `+`/`−` motor labels describe the connector wires, but the Robo Pico can reverse polarity to turn backward. They do **not** mean motor pin 5 goes directly to 5 V or pin 6 directly to ground. [PicoCar p. 1; Robo Pico pp. 5, 9]

The image does **not** give motor rated voltage, no-load/running/stall current, gear ratio, pulses per revolution, encoder output type, encoder supply voltage, wheel diameter, or track width. These are required for safe driver compatibility, encoder pull-ups/level shifting, speed in metres/second, travel distance, and angle control. Encoder counts must be measured at the **wheel**, including gearbox ratio and whether firmware counts one, two, or four edges per quadrature cycle. Both motors must be tested for polarity before treating `M1A/M1B` as physical forward. [PicoCar p. 1; Robo Pico pp. 5, 7, 9]

**Implementation implication:** use the encoder A/B pair on interrupt-capable GPIOs or a suitable PIO decoder, sample counts at a defined period, calculate speed from count difference and wheel geometry, close each wheel's speed loop, cap commands, and physically brake/disable on stop or timeout. The exact sample period, gains, and stopping distance need measurement.

For later calibration, define `N` as **decoded counts per wheel revolution** for the exact edge-counting mode, `D` as measured wheel diameter, and `b` as measured left-to-right wheel separation. Then wheel travel is `Δs = π D × Δcount/N`, wheel speed is `Δs/Δt`, and an approximate differential-drive heading change is `(Δs_right − Δs_left)/b` radians. These equations are derived engineering relationships, not stated in PicoCar.pdf. Measure `N` at the wheel because motor-shaft encoder counts alone omit gearing.

## IR reflectance sensing, line, and barcode

### Supplied device references describe different products

- [Infrared-Reflective-Sensor-UserManual.pdf](../../document/Infrared-Reflective-Sensor-UserManual.pdf) describes a **Waveshare ST188/LM393 module** with `DOUT`, `AOUT`, `GND`, and `VCC` (3.0–5.3 V). `AOUT` is a reflectance-related analogue signal and `DOUT` is thresholded by the onboard comparator. Its example wiring uses 3.3 V for an STM32 and 5 V for an Arduino; these are examples, not the team's pin choices. [pp. 1–2]
- [tcrt5000.pdf](../../document/tcrt5000.pdf) describes the **bare Vishay TCRT5000/TCRT5000L** emitter/phototransistor, not a powered four-pin module. It needs an LED current-limiting circuit and a detector load/conditioning circuit. Its peak operating distance is **2.5 mm**; its relative response remains above 20% over roughly **0.2–15 mm** in the stated test. The emitter wavelength is about 950 nm and the package has a visible-light filter. Actual black/white contrast depends strongly on mounting height and surface. [pp. 1–4]
- [LM393-D.PDF](../../document/LM393-D.PDF) describes the **comparator IC**, not a whole sensor module. It compares analogue voltages and its output needs a suitable pull-up/board circuit; a module powered at 5 V may present a 5 V digital pull-up even if the bare comparator can run over a broad supply range. Its IC supply range of 2–36 V is **not** permission to feed 5 V into Pico GPIO. Check the actual module's output high level and pull-up rail. [pp. 1–3]

The documents do not establish whether the team's three IR modules are ST188 modules, TCRT5000 modules, another model, or a mix. Do not assume a TCRT5000 chip datasheet proves the pinout of a four-pin board.

**Waveshare module information transcribed from its two-page manual:** the reflective element is listed as ST188, comparator as LM393, allowed module supply 3.0–5.3 V, board size 30.2 × 11.9 mm, and fixing-hole size 2.0 mm. Its connector order is **1 DOUT, 2 AOUT, 3 GND, 4 VCC**. The manual's STM32 example uses DOUT GPIOA.4, AOUT GPIOA.6 and 3.3 V supply; its Arduino example uses D2, A0 and 5 V. The example serial setting is 115200 baud, 8 data bits, no parity, 1 stop bit; this is the demonstration program, **not** a serial interface on the IR module. Its onboard indicator lights when close to a barrier in the manual's example. The manual does not state which DOUT logic level means black, the exact comparator threshold, or the best mounting height for this course. [Infrared manual pp. 1–2]

**Bare TCRT5000 limits and optics:** it combines an IR LED and phototransistor in a 10.2 × 5.8 × 7 mm leaded package; TCRT5000L has longer leads. The peak relative response occurs at about **2.5 mm** from a reflective target, and the quoted >20% relative-current distance range is **0.2–15 mm** under the datasheet's test conditions. The emitter has a visible-light blocking package, approximately 940–950 nm peak emission, about 1.25 V typical forward drop at a stated 60 mA test current, and **60 mA absolute maximum continuous forward current**; this maximum is not a recommended design current. The detector's stated collector-emitter maximum is 70 V and collector-current maximum 100 mA, but neither allows Pico GPIO to exceed 3.3 V. The sensor needs external LED current limiting and detector load/threshold circuitry; it is not interchangeable pin-for-pin with the Waveshare module. Reflectance changes with surface, height, temperature and LED current, so calibrate on the actual black line and white floor at the mounted height. [tcrt5000 pp. 1–4]

**LM393 comparator IC details:** two independent comparators are inside one 8-pin IC. On the bare IC, pins are `1=OUT A`, `2=IN− A`, `3=IN+ A`, `4=GND`, `5=IN+ B`, `6=IN− B`, `7=OUT B`, `8=VCC`. Its single-supply operating range is 2–36 V, but its input common-mode high end is about `VCC − 1.5 V` at 25 °C (or `VCC − 2.0 V` over the listed temperature range). At 5 V and 25 °C the LM393 family's typical supply current is 0.4 mA; the comparator can sink output current and needs an output pull-up to produce a high level. Pull-up **voltage**, not the broad chip VCC range, determines what a Pico digital input sees. Noise near the threshold can make DOUT chatter; the datasheet discusses adding a small amount of positive feedback (hysteresis) and keeping output traces away from sensitive inputs. If the module already contains these components, do not duplicate them blindly. [LM393 pp. 1, 3, 5]

| Question for line tracking | What to observe on the assembled sensor |
| --- | --- |
| Black vs white | Log `AOUT` if connected and `DOUT` on both surfaces; record the actual polarity. |
| Height | Compare black/white separation at the real chassis clearance, not the bare TCRT5000 datasheet's ideal test distance. |
| Lighting and speed | Test under the demonstration lighting and at slow/normal travel speeds. |
| Comparator threshold | Adjust or record the module's trim setting if present; use hysteresis/debounce where transitions chatter. |
| Electrical levels | Power module from a verified rail and measure DOUT HIGH before connecting to 3.3 V Pico GPIO. |

### Track and barcode geometry

[Barcode & Line Specification.pdf p. 1](<../../document/Barcode & Line Specification.pdf>) shows an **18 mm black line**, a **20 mm gap between the line edge and adjacent barcode**, and an **approximately 141 mm barcode length** along the illustrated direction. The measurements are on a photo; verify whether “gap” is from the line edge and whether 141 mm applies to all printed symbols on the physical track.

[Barcode Sample.pdf pp. 1–4](<../../document/Barcode Sample.pdf>) visually contains examples labelled **A** and **Z** (two renderings each). It does not give a symbology name, start/stop rules, width tolerances, scale, printed decode table, or samples for B/C/D. Therefore do not hard-code an assumed Code 39/other standard or equate A/Z examples with the assignment's A/B/C/D command example without a verified specification.

**Visual sample transcription, so the PDF need not be reopened for the bar structure:** raster inspection at 72 dpi found **15 black bars** on each page, arranged as three five-bar groups separated by narrow spaces. Reading across the bars, `n` means a narrow bar/space and `w` a wide bar/space. Each nine-element sequence below alternates **black bar, white space, black bar, ... , black bar**. The first and last groups are the same in A and Z, consistent with `*` start/stop framing in Code 39; that standard identification is an **inference from the shapes**, not a label printed in the source. The PDF's A/Z text and the run widths are directly observed. [Barcode Sample pp. 1–4]

| Sample pages | Relative printed scale | First group | Labelled middle group | Last group | Inter-character gap |
| --- | --- | --- | --- | --- | --- |
| 1 (A), 3 (Z) | Larger, about 2× the narrow width of pp. 2/4 | `nwnnwnwnn` | A: `wnnnnwnnw`; Z: `nwwnwnnnn` | `nwnnwnwnn` | Narrow |
| 2 (A), 4 (Z) | Smaller | `nwnnwnwnn` | A: `wnnnnwnnw`; Z: `nwwnwnnnn` | `nwnnwnwnn` | Narrow |

At the smaller scale, measured raster widths are approximately **10 px narrow** and **30 px wide** at 72 dpi; the larger pages show approximately 20 px and 60 px. These are PDF drawing/raster dimensions, **not guaranteed millimetres on the course**. The sample pages have quiet white margins, but no quiet-zone minimum or barcode tolerance is specified. The sample provides **A and Z only**; no printed B, C or D examples or final command mapping are supplied. Nicholas confirmed the examples are relevant to the actual track, while the chosen command symbols remain uncertain.

**Geometry sketch in words:** looking down at the track image, the continuous **18 mm-wide** black line runs parallel to the barcode lane; a **20 mm white gap** separates the bottom of the barcode bars from the nearest edge of the line; the photographed barcode occupies about **141 mm along the travel direction**. The image does not specify the sensor mounting coordinate, barcode-to-junction distance, direction of travel, or whether every symbol has identical printed length. [Barcode & Line Specification p. 1]

**`DOUT` versus `AOUT` for barcode decoding:** a correctly adjusted comparator `DOUT` is sufficient to observe the alternating black/white sequence. `AOUT` gives reflectance strength and can help calibrate a difficult surface or lighting condition, but a valid decoder does not require an analogue channel. **Counting only the black bars is insufficient:** both supplied A and Z samples contain 15 bars. Their identity is in the *ordered narrow/wide widths of bars and intervening white spaces*. In the current plan, use barcode `DOUT` on Grove 4 GP16 and leave barcode `AOUT` disconnected. [Barcode Sample pp. 1–4; Robo Pico p. 4]

For a moving single-sensor barcode read, each `DOUT` transition marks a black/white edge; the time or **wheel-encoder distance between edges** gives each bar/space width. Time alone changes with speed, so encoder distance or speed compensation is needed to distinguish narrow from wide. Record raw transitions over known A and Z samples, calibrate `DOUT` polarity and threshold on the actual print, confirm orientation/height and printed dimensions, define frame start/end and invalid-read rejection, then emit **one** navigation event per valid symbol. Test at both slow and normal speeds and reject broken or incomplete scans. Mount the line sensors over the line and the barcode sensor over the adjacent barcode lane; exact lateral offsets require a measured car drawing. Three sensors total may constrain simultaneous line-position estimation and independent barcode reading.

The current plan uses analogue reflectance from the two line sensors on GP26/ADC0 and GP27/ADC1; the barcode sensor is digital on GP16. Pico W exposes only GP26–GP28 as external ADC channels, and GP28 is the left encoder A digital input here. There is therefore no third free onboard ADC channel for barcode AOUT without changing the pinout or adding hardware. Calibrate both analogue line channels on the actual surface, lighting, and mounting height; calibrate the barcode comparator separately. [Pico W p. 14; Robo Pico p. 5]

## LSM303DLHC / GY-511 terrain sensor

[OKY3231-2.pdf](../../document/OKY3231-2.pdf) identifies a **GY-511 breakout carrying an LSM303DLHC** and advertises a 3–5 V module input. [PicoCar.pdf p. 2](../../document/PicoCar.pdf) labels its breakout connections `DRDY`, `INT2`, `INT1`, `SDA`, `SCL`, `GND`, `3.3`, and `Vin`. The breakout's regulator/pull-up circuit is not specified, so the safe supply pin and I²C high level must be verified on the physical board. The **chip itself** is specified for Vdd 2.16–3.6 V and is not a 5 V IC. [lsm303dlhc.pdf pp. 1, 12–14](../../document/lsm303dlhc.pdf)

The GY-511 product sheet additionally advertises 16-bit output, ±2/4/8/16 g accelerometer scales, ±1.3–8.1 gauss magnetometer scales, embedded temperature sensing, acceleration FIFO, low-power/power-down modes, orientation detection and programmable motion/free-fall interrupts. These describe chip capabilities; the breakout sheet does **not** specify a gyroscope, a circuit schematic, pull-up voltage, or the electrical meaning of both `Vin` and `3.3` headers. The pictured header order from one end is **DRDY, INT2, INT1, SDA, SCL, GND, 3.3, Vin**; verify board silkscreen orientation before wiring. [OKY3231-2 p. 1; PicoCar p. 2]

**Why only four GY-511 wires are proposed:** `Vin` plus `GND` powers the module, and `SDA` plus `SCL` lets the Pico configure and read all three acceleration axes and all three magnetic-field axes over I²C. The breakout's separate `3.3` pin is **not another axis or data connection**; the supplied breakout sheet does not establish whether it is regulator output or a bypass input. Power through the advertised 3–5 V module input (`Vin`) from Grove 3's 3V3 rail and leave `3.3` unconnected, rather than tying two power pins together. Verify that the installed board powers up and the I²C lines idle at no more than 3.3 V. `INT1` and `INT2` are optional, configurable accelerometer event outputs (for thresholds, motion/orientation, etc.); `DRDY` is an optional data-ready output. The firmware can poll the accelerometer status register and magnetic data-ready status over I²C before reading samples, so none of these three outputs is required for basic sensing. They can be added later if event-driven sampling is useful, with extra GPIO allocation and voltage checks. [lsm303dlhc pp. 9, 17, 23, 26–34, 39]

The LSM303DLHC contains a **3-axis accelerometer and 3-axis magnetometer**, with a temperature output and motion/orientation interrupt functions. It does **not** contain a gyroscope. Its accelerometer ranges are ±2/4/8/16 g; magnetic ranges ±1.3 to ±8.1 gauss. The I²C bus supports 100 and 400 kHz; the accelerometer uses 7-bit address **0x19**, the magnetometer **0x1E**. The two cores have separate register maps on the same bus. External I²C pull-ups are required at chip level; check whether the breakout already has them and that they pull to 3.3 V. [lsm303dlhc.pdf pp. 1, 10, 13, 19–23](../../document/lsm303dlhc.pdf)

**Electrical and data facts from the chip sheet:** chip Vdd operating range 2.16–3.6 V, Vdd_IO from 1.71 V up to approximately Vdd+0.1 V, operating temperature −40 to +85 °C; chip-level absolute maximum Vdd is 4.8 V, which is a damage limit and **not** a 5 V operating rating. I²C standard/fast clocks are up to 100/400 kHz with external pull-ups; the application example recommends **10 kΩ pull-ups** on SDA/SCL and local decoupling (4.7 µF, 0.22 µF, 100 nF and 10 µF capacitors at the bare chip). The assembled GY-511 may already contain relevant parts. High-current motor wiring can distort the magnetic field measurement; keep it away from the chip. [lsm303dlhc pp. 12–14, 17–18]

| Device | 7-bit I²C address | Corresponding 8-bit write/read address bytes | Practical note |
| --- | --- | --- | --- |
| Accelerometer | `0x19` | `0x32` / `0x33` | Set the register-subaddress high bit for multi-byte auto-increment reads. |
| Magnetometer | `0x1E` | `0x3C` / `0x3D` | Separate register map; data register order is X, Z, Y. |

Use **7-bit addresses** with Pico SDK I²C calls that expect 7-bit addresses; do not pass the 8-bit write byte in their place. [lsm303dlhc pp. 19–22]

| Accelerometer setting (`CTRL_REG1_A` ODR bits) | Normal/low-power output data rate |
| --- | ---: |
| `0000` | Power-down |
| `0001`, `0010`, `0011`, `0100` | 1, 10, 25, 50 Hz |
| `0101`, `0110`, `0111` | 100, 200, 400 Hz |
| `1000` | 1.620 kHz in low-power mode only |
| `1001` | 1.344 kHz normal or 5.376 kHz low-power |

`CTRL_REG1_A` defaults to `0x07`: X/Y/Z enable bits are set but ODR=`0000`, so configure a nonzero rate before expecting fresh acceleration samples. `CTRL_REG4_A` controls full scale, high-resolution mode and block-data-update (BDU); enabling BDU prevents high/low bytes from being mixed across an update. The datasheet lists nominal acceleration sensitivity of **1, 2, 4, 12 mg/LSB** for ±2, ±4, ±8, ±16 g respectively under its stated conditions. Data are signed two's complement; use the configured mode/scale when converting raw register values to g or m/s². [lsm303dlhc pp. 10, 23, 25, 27, 29]

| Magnetometer `CRA_REG_M` rate bits | Output rate |
| --- | ---: |
| `000`, `001`, `010`, `011` | 0.75, 1.5, 3, 7.5 Hz |
| `100`, `101`, `110`, `111` | 15, 30, 75, 220 Hz |

`CRB_REG_M` chooses the magnetic range/gain; `MR_REG_M` selects continuous, single-conversion or sleep mode. The register map lists `MR_REG_M` default `0x03` (sleep), so configure a measurement mode before expecting magnetic samples. At ±1.3 gauss, nominal gain is **1100 LSB/gauss for X/Y and 980 LSB/gauss for Z**; gains decrease at wider ranges. Temperature is enabled through `CRA_REG_M` and read from `TEMP_OUT_H_M`/`TEMP_OUT_L_M`; the sheet gives **8 LSB/°C** and a 12-bit two's-complement value, but no absolute-temperature accuracy claim suitable for hump measurements. [lsm303dlhc pp. 10, 24, 37–39]

| Needed operation | Register / fact | Source |
| --- | --- | --- |
| Enable acceleration axes and choose output rate | `CTRL_REG1_A` at 0x20 on 0x19; default axes enabled but **power-down rate** until configured. | [lsm303dlhc p. 25](../../document/lsm303dlhc.pdf) |
| Set accelerometer scale and coherent read | `CTRL_REG4_A` at 0x23; full scale, high-resolution mode, and block-data-update options. | [lsm303dlhc p. 27](../../document/lsm303dlhc.pdf) |
| Read acceleration | `OUT_X/Y/Z_L/H_A` at 0x28–0x2D; multi-byte accelerometer reads require the subaddress auto-increment bit. Convert signed data using the selected range/mode. | [lsm303dlhc pp. 21, 23](../../document/lsm303dlhc.pdf) |
| Configure/read magnetic field | `CRA_REG_M` 0x00 rate, `CRB_REG_M` 0x01 gain, `MR_REG_M` 0x02 mode; data registers 0x03–0x08 are ordered **X, Z, Y** and high byte before low byte. | [lsm303dlhc pp. 24, 37–39](../../document/lsm303dlhc.pdf) |
| Check freshness and temperature | Accelerometer status at 0x27, magnetic data-ready at 0x09; temperature output at 0x31/0x32 after enabling it. | [lsm303dlhc pp. 23–24, 28, 39](../../document/lsm303dlhc.pdf) |

**Implementation register map** (suffix `_A` belongs to I²C `0x19`; suffix `_M` belongs to I²C `0x1E`): [lsm303dlhc pp. 23–39]

| Address | Accelerometer register and role |
| --- | --- |
| `0x20` | `CTRL_REG1_A`: ODR, low-power enable and X/Y/Z enable. |
| `0x21` | `CTRL_REG2_A`: high-pass filter mode/cutoff and filtered-data selection. |
| `0x22` | `CTRL_REG3_A`: route click, interrupt and data-ready events to INT1. |
| `0x23` | `CTRL_REG4_A`: BDU, byte order, full-scale ±2/4/8/16 g and high-resolution mode. |
| `0x24` | `CTRL_REG5_A`: reboot memory content, FIFO enable and latched-interrupt options. |
| `0x25` | `CTRL_REG6_A`: route interrupt functions to second interrupt pin, polarity. |
| `0x26` | `REFERENCE_A`: high-pass/interrupt reference value. |
| `0x27` | `STATUS_REG_A`: new-data and overrun bits for axes. |
| `0x28–0x2D` | X low/high, Y low/high, Z low/high signed acceleration data. |
| `0x2E–0x2F` | FIFO control/source; bypass, FIFO, stream and trigger modes. |
| `0x30–0x33` | INT1 configuration/source/threshold/duration. |
| `0x34–0x37` | INT2 configuration/source/threshold/duration. |
| `0x38–0x3D` | Click/double-click configuration, source, threshold and timing. |

| Address | Magnetometer register and role |
| --- | --- |
| `0x00` | `CRA_REG_M`: output rate and temperature enable. |
| `0x01` | `CRB_REG_M`: magnetic full scale/gain. |
| `0x02` | `MR_REG_M`: continuous conversion, single conversion or sleep. |
| `0x03–0x08` | X high/low, **Z high/low**, **Y high/low** signed field data. |
| `0x09` | `SR_REG_M`: data ready and lock flags. |
| `0x0A–0x0C` | Identification registers; documented byte patterns `0x48`, `0x34`, `0x33`. |
| `0x31–0x32` | Temperature high/low, two's-complement data, when enabled. |

**Magnetic gain table** (configured range and nominal register counts per gauss): [lsm303dlhc pp. 10, 38]

| Range | X/Y LSB per gauss | Z LSB per gauss |
| ---: | ---: | ---: |
| ±1.3 gauss | 1100 | 980 |
| ±1.9 gauss | 855 | 760 |
| ±2.5 gauss | 670 | 600 |
| ±4.0 gauss | 450 | 400 |
| ±4.7 gauss | 400 | 355 |
| ±5.6 gauss | 330 | 295 |
| ±8.1 gauss | 230 | 205 |

Never write registers marked **reserved** or overwrite factory-calibration/trim contents. If using the chip's INT1/INT2 and FIFO later, configure threshold and duration relative to the chosen ODR and read the interrupt source register to clear a latched event. For a first driver, polling timestamped acceleration at a chosen ODR is simpler; the optional DRDY/INT wires are not required in the initial pinout. [lsm303dlhc pp. 23–24, 27–34]

**What the sensor can support:** pitch/roll from gravity when stationary or moving slowly; acceleration transients; a magnetic heading after calibration and separation from motor currents. It can help identify a climb/descent signature and sudden impacts. The source datasheet warns nearby high currents disturb magnetic readings. Mount it rigidly, mark its axes relative to forward/left/up, log stationary offsets, and test with motors running. [lsm303dlhc pp. 9, 15, 18]

**What it cannot directly report:** angular velocity or yaw rate, because there is **no gyro**. A heading difference divided by time is a derived magnetic estimate and can be poor beside motors or steel obstacles; encoder-based turning may be more reliable. Hump **height in metres** is also not a direct output. Pitch plus travelled distance could yield an estimate only if slope geometry, wheel contact, mounting, and slip are modeled and validated against known hump heights. Raw vertical acceleration double integration without drift control is not a dependable height measurement. Keep “detected hump” and “validated height estimate” as separate outputs.

## HC-SR04P ultrasonic ranging and servo scanning

**Installed part reported by Nicholas:** HC-SR04P, the 3.3 V-capable variant. An [HC-SR04P manufacturer listing](https://www.okystar.com/product-item/hc-sr04-ultrasonic-sensor-module-oky3261-2/38/) specifies a **3–5.5 V** operating range and lists **2–400 cm at 3.3 V** for that manufacturer's module. This supports the intended [architecture.md](ARCHITECTURE.md) connection of VCC to Grove 2 **3V3**, GND to Grove 2 GND, Echo to **GP2**, and Trig to **GP3**. Confirm the actual board is marked HC-SR04P (or has its own 3.3 V specification) before relying on that range; the manufacturer listing is representative, not proof of the exact installed board or its measured performance.

The supplied [HCSR04.pdf](../../document/HCSR04.pdf) describes a **different, older HC-SR04 that requires 5 V**. It remains a reference for the four-pin trigger/echo ranging principle: a ≥10 µs trigger, an eight-cycle 40 kHz acoustic burst, echo pulse width proportional to round-trip distance, and `distance_cm ≈ echo_high_us / 58`. Its approximately 15° beam and >60 ms measurement interval are conservative starting assumptions for planning, **not verified HC-SR04P specifications**. Measure the installed HC-SR04P's usable range, response, and minimum repeat interval on the car. [HCSR04 pp. 1–3]

At a 3.3 V supply, the intended direct Echo→GP2 connection is appropriate **only after** verifying the installed module's Echo HIGH remains at or below the Pico's 3.3 V GPIO limit; use a divider/level shifter if it does not. The 3.3 V Pico GP3 trigger should be tested with the installed module. A standard 5 V-only HC-SR04 must not be substituted into this wiring: it would need 5 V power and Echo level protection. Share ground, bound the echo wait, and treat a missing echo as **unknown/timeout**, never infinite clearance. Angle, smoothness, size, and material affect reflections, so a scan does not uniquely reveal exact object shape. [Pico W p. 14; HCSR04 pp. 1–3]

The Robo Pico provides servo signal ports GP12–GP15. **Nicholas identified the installed servo as a 9 g SG90**; it is not specified in the original 14 PDFs. [Cytron's SG90 listing](https://sg.cytron.io/p-sg90-micro-servo) gives brown=`GND`, red=`5 V`, orange=`Signal` and 4.8–5.0 V operating supply for the version it sells. Connect one three-wire plug to **servo port 1 / GP12** with orange on `S`, red on `+`, and brown on `−`; follow the actual board's silkscreen, not a presumed left/right orientation. With power off, verify that red aligns with `+` before seating the plug. The Robo Pico servo `+` follows its input power path, so measure that rail with the connected power bank and check the actual SG90 variant's rating. The supplied PDFs do not give this servo's current draw, stall current, calibrated angle range or center pulse. [Robo Pico pp. 4, 7]

A coarse-to-fine scan should wait for servo settling, keep each reading's angle/time, periodically refresh straight-ahead clearance, estimate left/right passability with car width and safety margin, and stop/retry when data are insufficient. Using the older HC-SR04 sheet's conservative >60 ms interval, five coarse readings span **more than 240 ms from first to fifth trigger**, plus servo settling; the HC-SR04P's actual timing should be tested. The robot should be stopped or moving under a separately justified safe condition during such scans. [Robo Pico p. 4; HCSR04 p. 2]

## Embedded C coding reference

[barr_c_coding_standard_2018.pdf](../../document/barr_c_coding_standard_2018.pdf) is a **coding-style and reliability reference**, not a hardware datasheet and not explicitly mandatory in either assignment PDF. It specifies **C99** as its language baseline and calls for minimizing compiler extensions, pragmas and assembly, particularly by isolating necessary hardware-specific code in driver modules. If the team chooses a different compiler language version, record that as a team decision rather than attributing it to this PDF. The following is a working summary of its enforceable rules and preferred practices, organized by the original document sections. [Barr pp. 20–27]

| Topic | Rules and implications for this firmware | PDF pages |
| --- | --- | --- |
| General formatting | Limit source lines to 80 characters. Always brace `if`, `else`, `switch`, `while`, `do` and `for` bodies, including single or empty statements; put the opening brace on the following line. Add parentheses rather than relying on obscure precedence, including around compound operands of `&&`/`\|\|`. Keep a version-controlled abbreviation list. Add a comment justifying the valid value range whenever a cast is used. | 21–25 |
| Keywords and storage | Do not use `auto` or `register`; avoid `goto` and `continue` where possible. Use `static` for items private to a module, `const` for immutable data and parameters, and `volatile` where appropriate for ISR/shared or memory-mapped state. `volatile` does not itself provide synchronization. | 26–27 |
| Comments | `//` and `/* */` are acceptable; do not leave commented-out code. Guard debug-only output with `#ifndef NDEBUG`. Write useful, complete comments about algorithm steps and assumptions, cite external algorithm references, keep needed diagrams under version control, and document modules/functions in a form suitable for generated documentation. Avoid comments that merely restate a line of C. | 29–30 |
| Spacing and layout | Put spaces around binary and assignment operators, not between unary operators and their operands. Align related declarations and assignments. Use one statement per line and blank lines around natural code blocks. Indent in multiples of four spaces; use no tab characters. Prefer LF line endings. The reference also asks for an end-of-file comment and blank line. | 33–40 |
| Modules and headers | Use lowercase, numeric and underscore module names; corresponding `.c` and `.h` files share a root name and have unique first eight characters. Give each source file one responsibility. Keep one guarded header for each source file, exposing only needed types, constants and prototypes; avoid `extern` data in headers. Include a module's own header from its source, never `#include` a `.c` file or absolute path, and remove unused includes. Keep project header/source templates. | 41–44 |
| Types and integers | Name new types with lowercase words and `_t`; prefix public types with the module name. Use `uint8_t`, `int16_t`, etc. when width matters. Do not use `short` or `long`; reserve `char` for strings. Do not do bitwise operations on signed values or mix signed and unsigned in expressions/comparisons; add `u` to unsigned constants. Use `bool` and relational expressions for Boolean conversion. Check byte padding and bit-field order before mapping structs to device registers or wire data. | 45–51 |
| Floating point | Avoid it if integer or fixed-point arithmetic suffices. Where used, the PDF asks for `float32_t`/`float64_t`/`float128_t`, `f` suffix on single-precision constants, a check that required double precision exists, no direct floating-point equality test, and `isfinite()` checks for computed values. These named float typedefs are a convention of this reference, not built-in C99 types; define or replace them explicitly in a real project. | 48 |
| Functions and macros | Use descriptive lowercase function names with underscores, a module prefix for public functions, and names no longer than 31 characters. Aim for functions of no more than 100 lines; declare each public prototype in its header, mark private functions `static`, and name all parameters. A single return at the end is preferred when it improves readability. Prefer functions over function-like macros; if a macro is necessary, parenthesize body/parameters, evaluate parameters once and do not transfer control from it. | 52–56 |
| Tasks and interrupts | Suffix RTOS entry functions with `_thread`, `_task` or `_process`. Name interrupt handlers with `_isr`, use the target compiler/SDK's ISR declaration mechanism, avoid calling an ISR like a normal function, and provide a default handler for unexpected interrupt sources where the platform permits. The Pico SDK's actual callback/IRQ registration API still governs implementation. | 57–58 |
| Variables | Use descriptive lowercase underscore names between 3 and 31 characters. Prefix globals with `g`, pointer names with `p`, and pointer-to-pointer names with `pp`. Initialize variables before use, set unassigned pointers to `NULL`, declare locals near first use, and group any globals near the top of their source file. | 60–62 |
| Control flow | Declare one variable per statement. Keep nested `if`/`else` no deeper than two levels; do not assign in `if` tests; end `if`/`else if` chains with `else`. Every `switch` gets `default`, and intentional case fallthrough gets a comment. Use named constants for loop limits, avoid assignment in loop tests, spell an infinite loop `for (;;)`, and explain empty loops. Avoid `abort()`, `exit()`, `setjmp()` and `longjmp()`. When comparing with `==`, put the constant on the left. | 63–68 |

The appendices provide an accepted-abbreviation list, header/source templates and a CRC example, followed by a bibliography and index. They are **examples**, not an assignment requirement to implement CRC or copy Barr Group's template copyright. [Barr pp. 69–86] For this project, decide which formatting and static-analysis rules the team will enforce, document any exceptions, and apply them consistently across drivers and buddy modules. The current skeleton has not been certified against this standard.

## Current Pico WH + Robo Pico connection plan

[architecture.md](ARCHITECTURE.md) contains Nicholas's consolidated intended wiring. It supersedes the earlier digital-IR/5 V HC-SR04 proposal; this summary does not assert that every wire has been physically verified. The Pico WH plugs into the carrier socket, so Pico physical pin numbers are not extra wires to attach. [Pico W p. 5; Robo Pico pp. 3–5]

| Part | Current intended connection | Check before use |
| --- | --- | --- |
| Power bank | Red→carrier VIN `+`, black→VIN `−` | Measure polarity, voltage, and available current under load. |
| Left motor | Yellow→M1B, black→M1A; carrier controls GP8/GP9 | Confirm forward polarity and motor current. Motor leads are not encoder leads. |
| Right motor | Yellow→M2B, black→M2A; carrier controls GP10/GP11 | Confirm forward polarity and motor current. |
| Right encoder | Grove 1: green B→GP0, red A→GP1; white→3V3, blue→GND | Verify supply/output voltage and decoded direction/counts. |
| Left encoder | Grove 7: green B→GP7, red A→GP28; white→3V3, blue→GND | GP28 is used digitally; keep optional battery-sense jumper open. |
| Right line IR | Grove 6: AOUT→GP27, 3V3, GND | GP27 is ADC1; leave DOUT and Grove 6’s shared GP26 contact disconnected. |
| Left line IR | Grove 5: AOUT→GP26, 3V3, GND | GP26 is ADC0; leave DOUT disconnected. |
| Barcode IR | Grove 4: DOUT→GP16, 3V3, GND | Leave AOUT disconnected; verify DOUT polarity and HIGH level. |
| GY-511 | Grove 3: SDA→GP4, SCL→GP5, `Vin`→3V3, GND | Verify board header and ≤3.3 V I²C pull-ups; leave breakout `3.3` pin unconnected. |
| HC-SR04P | Grove 2: VCC→3V3, GND→GND, Echo→GP2, Trig→GP3 | Verify exact 3.3 V-capable part and Echo HIGH ≤3.3 V before connecting GP2. Standard 5 V HC-SR04 is not interchangeable. |
| SG90 servo | Servo port 1: orange→GP12 `S`, red→`+`, brown→`−` | Check actual lead colors, silkscreen, and servo rail voltage/current. |

**Shared resources:** Grove 5 and Grove 6 both expose GP26; only the left IR uses that signal. Grove 2 and the Maker port share GP2/GP3; do not attach another device to that Maker port. GP28 is committed to the left encoder and cannot simultaneously sense carrier battery voltage. The two line IR sensors use analogue AOUT, while the barcode IR uses digital DOUT. The carrier's combined Grove 3V3 output limit is 300 mA; measure the total sensor load. [Robo Pico pp. 4–7]

### Checks before energizing or calibrating

1. Measure the power-bank red-to-black voltage and polarity with the board disconnected. Confirm the carrier VIN and servo rail under load and compare each motor's current against the carrier's 1 A continuous / 1.5 A brief peak per-channel limits.
2. Identify the actual HC-SR04P marking and test VCC at Grove 2 3V3. Measure Echo HIGH without GP2 attached; direct connection is acceptable only if it stays within the Pico's 3.3 V GPIO range. Test GP3's trigger and range with the wheels raised. If the module is a 5 V-only HC-SR04, redesign its supply and Echo interface instead of following this map.
3. Check actual IR-board pin order and measure AOUT/DOUT levels; do not assume a four-wire sensor header matches Grove contact order. Ensure Grove 6's shared GP26 contact is unused. Confirm encoder A/B supply/output levels and direction by turning each wheel manually. Leave the GP28 battery-sense jumper open.
4. Verify GY-511 `Vin`, GND, SDA and SCL against its silkscreen and that I²C idle levels are no more than 3.3 V. Do not connect both `Vin` and `3.3` without a verified board schematic.
5. Record wheel diameter, wheel separation, decoded counts per wheel revolution, IR mounting height/spacing, barcode dimensions and command mapping, servo zero angle, ultrasonic range/timing, and measured hump reference heights. These are calibration inputs, not values established by the PDFs.
