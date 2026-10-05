# Hardware and pin assignments

Primary design: [CSC01 Design Review, Section 2](<../../CSC01 Design Review.docx>). Detailed electrical evidence is included below. Platform: **Raspberry Pi Pico WH on Cytron Robo Pico**, SDK `PICO_BOARD=pico_w`. Maker Pi Pico in the former code documents was stale information.

## Submitted integrated design versus current bench

| Device | Review connection | Current right-IR bench |
| --- | --- | --- |
| Left motor M1 | Black → M1A, yellow → M1B; internal GP8/GP9 | Used; verify physical forward polarity |
| Right motor M2 | Black → M2A, yellow → M2B; internal GP10/GP11 | Used; verify physical forward polarity |
| Right encoder | Grove 1: green B GP0, red A GP1, white supply, blue GND | Unused |
| Left encoder | Grove 7: green B GP7, red A GP28, white supply, blue GND | Unused; GP28 battery jumper stays open |
| Right line IR | Grove 5: AOUT GP26/ADC0, VCC 3V3, GND | **Swapped to Grove 6 GP27/ADC1** |
| Left line IR | Grove 6: AOUT GP27, VCC 3V3, GND | **Swapped to Grove 5 GP26/ADC0**, unused |
| Barcode IR | Grove 4: DOUT GP16, VCC 3V3, GND | Unused |
| GY-511 / LSM303DLHC | Grove 3: SDA GP4, SCL GP5, Vin 3V3, GND | Disconnected; wires reused for motors |
| HC-SR04P | Grove 2: Echo GP2, Trig GP3, VCC 3V3, GND | Disconnected; wires reused for motors |
| SG90 | Servo 1: orange signal GP12, red +, brown − | Disabled; GP12 LOW, no pulses |
| Local start/reset | Onboard GP20 | Fresh press starts Ready; Stopped reset requires separate new start press |
| Local stop | Onboard GP21 | Brakes and latches Stopped |
| Power bank | Red/black → green VIN +/− | Same intended supply; actual rails/load require measurement |

The Word table labels left GP27 as ADC0; **GP27 is ADC1**. Preserve the document as submitted and use the physical mapping above. Do not silently revert the current GPIO swap to match the submitted baseline. Firmware/header and [bench guide](architecture/LINE_FOLLOWING.md) specify what to wire for the active UF2.

## Electrical and shared-resource constraints

- Motors connect to M1/M2 screw terminals, never sensor Grove GPIO or 3V3. Extension-wire colors were not supplied; follow motor-lead colors and terminal labels.
- Pico GPIO/ADC and I2C highs must stay within 3.3 V. Measure the installed HC-SR04P Echo HIGH before connecting GP2; a standard 5 V HC-SR04 is not interchangeable.
- Grove 5 and 6 share GP26. Only the GP26 line sensor uses it; leave Grove 6’s shared GP26 contact disconnected.
- Grove 2 shares GP2/GP3 with the Maker port. GP28 encoder use excludes the optional battery-sense jumper.
- Aggregate Grove 3V3 budget: 300 mA. Each motor channel: 1 A continuous, 1.5 A peak under 5 s. Check actual loads and VIN/servo rail against installed parts.
- GY-511 contains accelerometer and magnetometer, **no gyroscope**. Physical hump height requires a reference-validated method.

These are intended connections and cited limits, not blanket proof of installed wiring, actual currents or successful subsystem tests. Exact motor/IR/encoder variants and measured geometry remain unresolved. Track geometry and final barcode mapping are recorded in [barcode architecture](architecture/BARCODE.md).


## Source inventory



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

## Controller and carrier reference

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

## Electrical release checks



1. Measure the power-bank red-to-black voltage and polarity with the board disconnected. Confirm the carrier VIN and servo rail under load and compare each motor's current against the carrier's 1 A continuous / 1.5 A brief peak per-channel limits.
2. Identify the actual HC-SR04P marking and test VCC at Grove 2 3V3. Measure Echo HIGH without GP2 attached; direct connection is acceptable only if it stays within the Pico's 3.3 V GPIO range. Test GP3's trigger and range with the wheels raised. If the module is a 5 V-only HC-SR04, redesign its supply and Echo interface instead of following this map.
3. Check actual IR-board pin order and measure AOUT/DOUT levels; do not assume a four-wire sensor header matches Grove contact order. Ensure Grove 6's shared GP26 contact is unused. Confirm encoder A/B supply/output levels and direction by turning each wheel manually. Leave the GP28 battery-sense jumper open.
4. Verify GY-511 `Vin`, GND, SDA and SCL against its silkscreen and that I²C idle levels are no more than 3.3 V. Do not connect both `Vin` and `3.3` without a verified board schematic.
5. Record wheel diameter, wheel separation, decoded counts per wheel revolution, IR mounting height/spacing, barcode dimensions and command mapping, servo zero angle, ultrasonic range/timing, and measured hump reference heights. These are calibration inputs, not values established by the PDFs.
