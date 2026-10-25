# Technical and hardware information

Project allocations are in [TEAM](TEAM.md#2-pins); operating power/flash steps are in [GETTING_STARTED](GETTING_STARTED.md#power-and-flashing). This guide owns component facts and driver reference data. Chip absolute maxima, module ratings and measured system limits are different. Unverified installed-part/calibration values are tracked once in [PROJECT_SPECS](PROJECT_SPECS.md#pending-decisions).

## Raspberry Pi Pico W / WH

WH is the header-equipped W. The SDK board is `pico_w`.

| Item | Specification / use |
| --- | --- |
| Processor | RP2040 dual Arm Cortex-M0+, up to 133 MHz; template uses 125 MHz |
| Memory | 264 KB SRAM, 2 MB board flash |
| Interfaces | 26 header-accessible GPIO; 2 UART, 2 SPI, 2 I2C; 8 PWM slices / 16 channels; 2 PIO blocks / 8 state machines; USB 1.1 |
| ADC | 12-bit, silicon maximum 500 ksample/s; three externally available analogue pins, one internal channel; reference/supply noise affects readings |
| Radio | CYW43439, 2.4 GHz 802.11n WiFi and Bluetooth 5.2 hardware; project uses WiFi, not Bluetooth |
| Size / environment | 51 × 21 × 1 mm board; operating ambient −20 to +70 °C; keep metal away from the approximately 14 × 9 mm antenna area |
| Programming | ROM BOOTSEL UF2 or SWD; onboard wireless LED is on `WL_GPIO0`, not an ordinary RP2040 GPIO |

### Power and logic

| Signal / physical pin | Limit / purpose |
| --- | --- |
| VBUS / 40 | USB 5 V ±10% input |
| VSYS / 39 | Board input 1.8–5.5 V; this is not the Robo carrier VIN rating |
| 3V3 OUT / 36 | Regulated output; recommended external load below 300 mA, with available current depending on board/radio load |
| 3V3_EN / 37 | Regulator enable |
| ADC_VREF / 35, AGND / 33 | ADC reference and analogue ground |
| RUN / 30 | Active-low reset input |
| GPIO / ADC inputs | 3.3 V domain; never apply 5 V |

ADC_VREF is filtered from 3V3 through approximately 201 Ω / 2.2 µF; loading can produce about a 30 mV offset and supply noise. Averaging improves repeatability but does not establish absolute accuracy. No precision external reference is configured in this project.

Radio/internal signals include power enable, bidirectional data/IRQ, chip-select and a shared internal clock/VSYS-divided sensing path. They are reserved in [TEAM](TEAM.md#2-pins); do not repurpose them or sample internal supply signals while disrupting the radio.

### Physical header map

Physical header numbers differ from GPIO numbers. This is the W/WH electrical map; project functions remain assigned only in TEAM.

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

### Useful RP2040 driver registers

| Block | Registers / constraints |
| --- | --- |
| ADC | `CS` channel/enable/start/ready/error, `RESULT` 12-bit result, `FCS`/FIFO optional. A completed two-channel read must carry one coherent acquisition time. |
| PWM | Per-slice `CSR`, `DIV`, `CTR`, `CC`, `TOP`; frequency = system clock / (divider × (TOP + 1)) in free-running mode. Preserve other slices and shared reset ownership. |
| SIO / GPIO | Input/output and atomic SET/CLR registers; configure function/pulls/direction explicitly. Bounded edge handlers timestamp transitions. |
| TIMER / watchdog | Preserve the existing monotonic timebase; use wrap-safe elapsed subtraction. Configure watchdog tick without resetting a timer used by services. |

See subsystem driver contracts for acquisition and control policy; hardware capability does not imply that a driver is enabled.

## Cytron Robo Pico carrier

| Item | Carrier rating / behavior |
| --- | --- |
| VIN | 3.6–6 V |
| Grove 3V3 rail | 300 mA maximum total external load; budget all attached devices together |
| Motor channels | 1 A continuous per channel; 1.5 A peak for less than 5 s; supported PWM up to 20 kHz |
| Analogue input | 0–3.3 V |
| Digital input | LOW −0.3–0.8 V, HIGH 2.0–3.6 V; do not interpret this as 5 V tolerance |
| Digital output | LOW 0–0.5 V, HIGH 2.6–3.3 V under specified conditions |
| Motor / servo supply | VIN-only: approximately VIN; USB-only: approximately VUSB − 0.4 V; measure under load |

| Input A | Input B | Driver response |
| --- | --- | --- |
| LOW | LOW | Brake |
| HIGH | LOW | Forward electrical direction |
| LOW | HIGH | Reverse electrical direction |
| HIGH | HIGH | High-impedance/coast |

Physical forward direction depends on motor wiring and installation. Brake is an electrical request, not proof of immediate stationarity. Check wiring with power off; share grounds and avoid motor-current return paths through sensor ground wiring. Sensor/servo currents and motor stall current must fit the actual rail/driver limits.

The carrier provides seven Grove ports, motor terminals, four servo signal ports, local buttons, buzzer/mute switch, RGB LEDs and a battery-measurement jumper. Signal pair assignments/sharing belong to [TEAM](TEAM.md#2-pins). The optional battery divider yields `VBAT / 2` when its solder jumper is closed; the selected VBAT follows the highest relevant supply. The carrier switch cuts power, reset restarts the Pico, and motor-test buttons drive the motors at full speed independently of the application request path. Keep wheels clear and avoid those buttons while validating unknown wiring. A Maker Pi Pico is a separate development carrier and has no equivalent Robo motor-driver assumption.

## Motors and encoders

The supplied car connector is **PH2.0, six pins**:

| Pin | Function |
| --- | --- |
| 1 | Encoder ground |
| 2 | Encoder A |
| 3 | Encoder B |
| 4 | Encoder supply |
| 5 | Motor + |
| 6 | Motor − |

The connector drawing does not establish motor voltage/current, stall current, gear ratio, encoder electrical output, pulses/revolution or car geometry. Do not infer these from connector colors. Use the project's actual wire map in TEAM and measurements in PROJECT_SPECS.

For decoded counts per wheel revolution `N`, wheel diameter `D`, interval `dt` and track width `b`:

- Wheel travel: `ds = pi * D * delta_count / N`.
- Wheel speed: `v = ds / dt`.
- Differential-drive yaw change: `dtheta = (ds_right - ds_left) / b` radians; convert to degrees for the public contract.

`N` includes the selected edge decoding and gearing; raw motor-shaft PPR cannot be substituted without conversion. Direction comes from quadrature phase. Account for wrap, bounce/noise, missed edges, slip and finite measurement windows. [Motion](architecture/MOTION.md) owns PID/actions.

## TCRT5000-based IR modules

The installed sensor family is **TCRT5000-based**. An IR LED illuminates the surface; a phototransistor senses reflected light. A complete board adds current limiting, a load network and potentially a comparator/adjustment potentiometer. AOUT represents analogue reflectance; DOUT is a thresholded comparator output. Neither output polarity nor a board supply rating follows automatically from the bare sensor.

### Bare TCRT5000 facts

| Item | Component specification |
| --- | --- |
| Package | Approximately 10.2 × 5.8 × 7 mm; emitter/detector with visible-light suppression |
| Optical behavior | Near-infrared around 940–950 nm; response peaks around 2.5 mm under test conditions |
| Relative range | Roughly 0.2–15 mm at more than 20% relative response under stated test conditions; not guaranteed reliable module detection distance |
| LED | Typical forward drop 1.25 V at the 60 mA characterization point; 60 mA is an absolute maximum, not a chosen drive current; reverse maximum 5 V |
| Phototransistor | Collector-emitter maximum 70 V and collector current maximum 100 mA; these do not permit high voltage on Pico inputs |
| Other maxima | Component dissipation 200 mW; operating ambient −25 to +85 °C |

Keep LED current within a justified resistor/current budget. Mounted height, surface, lighting, alignment and board circuitry determine useful contrast. The different ST188-based module's advertised voltage, size and detection distance do not describe these boards.

### LM393 comparator reference

If the installed board uses LM393, it is a dual comparator with **open-collector outputs**. The pull-up rail determines DOUT HIGH; a high supply does not guarantee a safe Pico output. Verify both AOUT and DOUT levels at the actual board supply.

| Item | Bare IC information |
| --- | --- |
| Supply | 2–36 V operating range; does not define the module's safe input range |
| Common-mode inputs | Ground to approximately VCC − 1.5 V at 25 °C, VCC − 2 V across temperature |
| Typical supply current | About 0.4 mA at 5 V under stated conditions |
| Output saturation | Typically 150 mV, maximum 400 mV at 4 mA / 25 °C |
| Response | Approximately 1.3 µs under specified overdrive/load conditions; board filtering/hysteresis changes behavior |
| Eight-pin order | 1 OUT A, 2 IN− A, 3 IN+ A, 4 GND, 5 IN+ B, 6 IN− B, 7 OUT B, 8 VCC |

Threshold chatter may need justified hysteresis/filtering. Comparator trim is separate from analogue black/white calibration. Do not connect unused AOUT/DOUT contacts merely because they are present; follow TEAM's allocation. [Line Tracking](architecture/LINE_TRACKING.md) and [Barcode](architecture/BARCODE.md) own the processing logic.

## Ultrasonic sensor

The installed part is reported as **HC-SR04P**. A representative P module is rated 3–5.5 V and 2–400 cm at 3.3 V; this is not proof of the exact installed board. Confirm its marking, supply rating and Echo HIGH at or below 3.3 V before direct Pico connection. A 5 V-only HC-SR04 substitution requires appropriate power and Echo level protection.

The older standard HC-SR04 reference specifies 5 V, about 15 mA, nominal 2 cm–4 m range, approximately 3 mm accuracy, a roughly 15° beam and an eight-cycle 40 kHz burst after a trigger of at least 10 µs. Those variant-specific performance figures are not guaranteed for the reported P board.

Echo HIGH is round-trip time: `distance = sound_speed * echo_seconds / 2`; nominal room-temperature conversion is `distance_cm ≈ echo_us / 58`. Temperature and target angle/material/size affect results. Begin repeat-rate planning conservatively above 60 ms, then validate the installed module; bound echo waits and distinguish timeout/unknown from clear space. A scan samples reflected surfaces rather than reconstructing exact object shape.

## SG90 servo

Reported installed part: 9 g SG90. A commonly sold variant specifies 4.8–5.0 V, approximately 1.8 kg·cm stall torque, 0.12 s per 60° and nominal 180° travel, with an approximately 23 × 12.2 × 29 mm body. Apply these only after matching the actual variant; stall current is not established.

A PWM pulse commands position; there is no measured angle feedback. Nominal starting values are 50 Hz, center 1500 µs and 1000–2000 µs span. Calibrate endpoints/center and settling without forcing mechanical stops. Removing pulses is not a mechanical brake. Connector colors and project signal allocation are in TEAM. Measure the carrier servo rail under load and check the installed servo's rating/current budget.

## GY-511 / LSM303DLHC IMU

GY-511 carries a **three-axis accelerometer and three-axis magnetometer**, with temperature sensing. It has **no gyroscope**. Turn rate must be an encoder-derived estimate or another explicitly justified estimate. Product advertising of 16-bit output describes register words; accelerometer effective resolution depends on mode.

### Electrical and interface facts

- Breakout advertising gives 3–5 V input. Header order shown is `DRDY, INT2, INT1, SDA, SCL, GND, 3.3, Vin`; verify physical orientation. Its regulator and pull-ups are not established by that label alone. Follow TEAM's four-wire Vin/GND/SDA/SCL plan; verify operation and idle bus level. Leave the separate `3.3` pin alone until its electrical role is confirmed.
- Bare chip VDD: 2.16–3.6 V; VDD_IO: 1.71 V to approximately VDD + 0.1 V. Absolute maximum 4.8 V is not a normal 5 V operating rating. Operating temperature: −40 to +85 °C. A stated approximately 110 µA consumption is mode/rate-specific.
- I2C supports 100/400 kHz. API **seven-bit addresses** are accelerometer `0x19`, magnetometer `0x1E`; corresponding bus write/read bytes are `0x32/0x33` and `0x3C/0x3D`.
- Chip-level pull-ups are required; 10 kΩ is a reference starting value. Check existing breakout pull-ups and capacitance before adding more. Keep high-current paths and magnetic objects away from the magnetometer.
- Poll data-ready status before reading, or allocate validated interrupt wiring. A repeated cached read is not fresh acquisition. Never write reserved/factory registers.

### Accelerometer driver reference

| Register | Purpose |
| --- | --- |
| `CTRL_REG1_A 0x20` | Output rate, low-power selection and axes; reset `0x07` enables axes but leaves acquisition powered down |
| `CTRL_REG4_A 0x23` | Block-data update, full scale and high resolution |
| `STATUS_REG_A 0x27` | Data-ready / overrun flags |
| `OUT_X_L_A` through `OUT_Z_H_A 0x28–0x2D` | XYZ, low then high bytes; combine signed words and decode selected mode |
| `FIFO_CTRL/STATUS 0x2E/0x2F` | Optional acceleration FIFO configuration/status |
| Interrupt / click groups | Optional motion/orientation/free-fall/click events; threshold/duration need calibrated configuration |

For a burst, set the subaddress auto-increment bit. In high-resolution mode the sample is signed, left-aligned **12-bit**; arithmetic shift by four after combining bytes. Normal/low-power modes use different effective resolution. Standard rates include 1, 10, 25, 50, 100, 200 and 400 Hz; higher rates depend on mode.

| Full scale | High-resolution sensitivity |
| --- | --- |
| ±2 g | 1 mg/LSB |
| ±4 g | 2 mg/LSB |
| ±8 g | 4 mg/LSB |
| ±16 g | 12 mg/LSB |

### Magnetometer driver reference

| Register | Purpose |
| --- | --- |
| `CRA_REG_M 0x00` | Rate / temperature enable |
| `CRB_REG_M 0x01` | Gain |
| `MR_REG_M 0x02` | Reset `0x03` is sleep; `0x00` selects continuous acquisition |
| Outputs `0x03–0x08` | Signed high/low byte order **X, Z, Y**; reorder to XYZ in the API |
| `SR_REG_M 0x09` | Ready / lock |
| ID `0x0A–0x0C` | Expected `0x48, 0x34, 0x33` |
| Temperature `0x31/0x32` | Signed left-aligned 12-bit, 8 LSB/°C; not a calibrated absolute-temperature result |

Rates: 0.75, 1.5, 3, 7.5, 15, 30, 75 and 220 Hz. Reject the `−4096` overflow code.

| Magnetic full scale (gauss) | XY sensitivity (LSB/gauss) | Z sensitivity |
| --- | ---: | ---: |
| ±1.3 | 1100 | 980 |
| ±1.9 | 855 | 760 |
| ±2.5 | 670 | 600 |
| ±4.0 | 450 | 400 |
| ±4.7 | 400 | 355 |
| ±5.6 | 330 | 295 |
| ±8.1 | 230 | 205 |

Gravity-derived pitch/roll is useful only after mounted-axis/bias calibration and when acceleration is near gravity. Dynamic acceleration, slip and magnetic disturbances need validity guards. [Terrain](architecture/TERRAIN.md) owns classification, height validation and peak retention.

## Barcode samples

Supplied printed samples are **A and Z**, each at two scales; the larger is about twice the smaller. Both contain 15 black bars in three groups of five, so counting bars cannot distinguish them.

Each sequence alternates black bar / white space, starting and ending with a black bar. `n` = narrow, `w` = wide; inter-character gaps are narrow. The observed wide:narrow ratio is approximately 3:1, not a calibrated tolerance.

| Sample | First group | Middle group | Last group |
| --- | --- | --- | --- |
| A | `nwnnwnwnn` | `wnnnnwnnw` | `nwnnwnwnn` |
| Z | `nwnnwnwnn` | `nwwnwnnnn` | `nwnnwnwnn` |

The patterns are consistent with Code 39 `*A*` and `*Z*`; this is an inference, not a confirmed symbology specification. Width tolerances, quiet-zone minimum, mounting coordinate and junction offset remain pending ([PROJECT_SPECS](PROJECT_SPECS.md#pending-decisions) D01/D02).
