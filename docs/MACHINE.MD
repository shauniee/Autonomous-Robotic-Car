# Machine summary

Design authority: [CSC01 Design Review](<../../CSC01 Design Review.docx>). Wiring and limits: [HARDWARE.md](HARDWARE.md), [technical reference](tech-info.md).

| Item | Design / known information |
| --- | --- |
| Controller / carrier | Raspberry Pi Pico WH / Cytron Robo Pico; SDK `pico_w` |
| Motion | Two DC gear motors, M1 left / M2 right; exact motor/encoder models and geometry unverified |
| Line / barcode | Two analogue line IR channels plus separate digital GP16 barcode input; exact installed IR model unverified |
| Terrain | GY-511 / LSM303DLHC accelerometer + magnetometer; no gyro |
| Obstacle | Reported HC-SR04P and SG90; validate installed variant, voltage and settling |
| WiFi | Pico WH onboard CYW43; no external radio |
| Supply | Reported power bank; voltage/current and load budget require measurement |
| Track | Source photo: 18 mm line, 20 mm line-to-barcode gap, approximately 141 mm barcode length; verify physical scale |
| Barcode mapping | A/B/C/D → left/right/straight/U-turn is an assignment example, not a confirmed course decode table. Supplied samples are A/Z |
| Sensor layout | Full design uses left/right line sensors and separate barcode sensor; exact chassis positions require measurement |

Current bench: right IR Grove 6 GP27/ADC1 and M1/M2 only; left IR Grove 5 GP26/ADC0 unused, servo disabled, IMU/sonar disconnected, encoders unused. See [current operation](RIGHT_IR_LINE_FOLLOW.md). The review’s submitted line-IR ports are the opposite; both are documented in the hardware table.
