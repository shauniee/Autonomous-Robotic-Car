# Hardware and pin assignments

Primary design: [CSC01 Design Review, Section 2](<../../CSC01 Design Review.docx>). Detailed electrical evidence: [tech-info.md](tech-info.md). Platform: **Raspberry Pi Pico WH on Cytron Robo Pico**, SDK `PICO_BOARD=pico_w`. Maker Pi Pico in the former code documents was stale information.

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

The Word table labels left GP27 as ADC0; **GP27 is ADC1**. Preserve the document as submitted and use the physical mapping above. Do not silently revert the current GPIO swap to match the submitted baseline. Firmware/header and [bench guide](RIGHT_IR_LINE_FOLLOW.md) specify what to wire for the active UF2.

## Electrical and shared-resource constraints

- Motors connect to M1/M2 screw terminals, never sensor Grove GPIO or 3V3. Extension-wire colors were not supplied; follow motor-lead colors and terminal labels.
- Pico GPIO/ADC and I2C highs must stay within 3.3 V. Measure the installed HC-SR04P Echo HIGH before connecting GP2; a standard 5 V HC-SR04 is not interchangeable.
- Grove 5 and 6 share GP26. Only the GP26 line sensor uses it; leave Grove 6’s shared GP26 contact disconnected.
- Grove 2 shares GP2/GP3 with the Maker port. GP28 encoder use excludes the optional battery-sense jumper.
- Aggregate Grove 3V3 budget: 300 mA. Each motor channel: 1 A continuous, 1.5 A peak under 5 s. Check actual loads and VIN/servo rail against installed parts.
- GY-511 contains accelerometer and magnetometer, **no gyroscope**. Physical hump height requires a reference-validated method.

These are intended connections and cited limits, not blanket proof of installed wiring, actual currents or successful subsystem tests. Exact motor/IR/encoder variants and measured geometry remain unresolved. Track geometry and final barcode mapping are recorded with evidence boundaries in [technical reference](tech-info.md).
