# Hardware and pin planning

Communications target confirmed in the conversation: **Raspberry Pi Pico W**.
`MACHINE.MD` lists ROBO PICO and MAKER PI PICO carrier boards; verify that the
mounted module is the W variant. Peripheral models and wiring still need
confirmation. No external GPIO map is embedded in the code.

| Part | Exact model | Interface / signal needs | Proposed GPIOs | Owner |
| --- | --- | --- | --- | --- |
| Controller | Pico W; confirm mounted module | USB, GPIO, onboard WiFi | TBD | Integration |
| Motor driver | TBD | PWM/direction/enable per actual driver | TBD | Buddy 2 |
| Left/right encoders | TBD | Digital edge inputs; confirm channel count | TBD | Buddy 2 |
| 3 IR sensors | TBD | Analogue or digital output; confirm model | TBD | Buddy 3 |
| IMU | TBD | I2C or SPI; confirm address and axes | TBD | Buddy 4 |
| Ultrasonic sensor | TBD | Trigger/echo or model-specific interface | TBD | Buddy 5 |
| Servo | TBD | Control pulse and separate power requirements | TBD | Buddy 5 |
| WiFi | Pico W onboard CYW43 | Managed by Pico SDK | No external pin assignment | Buddy 1 |
| Battery/regulators | TBD | Voltage/current budget and common reference | N/A | Whole team |

Before selecting pins, check the actual part datasheets and Pico pin functions,
PWM resource sharing, I2C/SPI/UART availability and ADC needs. Record those source
links with the chosen components. Verify supply and GPIO logic levels before
connecting any sensor output, especially ultrasonic echo and encoder outputs.
Motors require a driver and a suitable supply; GPIO pins are control signals.

For the first powered motion test, keep wheels raised and verify the physical
stop behavior before trying low-speed runs on the course. Record wheel diameter,
track width, encoder counts per revolution and gearing for distance/turn control.

Official references:

- [Pico board variants and pinout resources](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)
- [Pico SDK setup](https://github.com/raspberrypi/pico-sdk)
- [Official hardware examples](https://github.com/raspberrypi/pico-examples)
