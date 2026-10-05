# Design review consistency audit — 5 October 2026

Primary source: [CSC01 Design Review.docx](<../../CSC01 Design Review.docx>), inspected as OOXML text/tables. The Word file was not edited; SHA-256 before/after: `81f5f55d275c46e748d4e44530fec3c3b1efc908a3e8b1e10b43554590b5f4cf`. This audit verifies documented content against source/code; it is not a new hardware test or visual review of the Word document’s drawings.

## Findings and resolutions

| Difference | Review authority / resolution |
| --- | --- |
| Root architecture versus code architecture | Consolidated into `ARCHITECTURE.md`: review target, proposed tasks/contracts and separate current implementation status |
| Maker Pi Pico versus Robo Pico | Review Section 2 and wiring figure identify Robo Pico; hardware/machine/getting-started corrected |
| Cooperative loop presented as final architecture | Review Section 3 proposes FreeRTOS Safety P5, Motion P4, Mission/Line P3, Terrain/Obstacle P2, Communications P1. Cooperative bench remains an implementation distinction |
| Integration owner unassigned / Shawn spelling | Review ownership table assigns Nicholas integration/Safety/Mission; communications owner is Shaun |
| MQTT start/stop, default stop on disconnect | Review FR12 and Section 4.3 permit status only; reject drive/start/stop; healthy local autonomy continues on loss. Current bench’s stopped calibration is a later user-requested extension |
| Right/left IR ports differ | Review Section 2: right GP26/Grove 5, left GP27/Grove 6. Current user-requested bench swap: right GP27/Grove 6, left GP26/Grove 5. Both recorded; firmware unchanged |
| Review labels GP27 ADC0 | Physical GP27 is ADC1; technical/firmware mapping uses ADC1, while documenting the submitted typo |
| Stale TBD hardware table | Updated reported GY-511, HC-SR04P, SG90, motor GPIO and local buttons; exact variants/measurements remain unverified |
| Fixed A/B/C/D mapping in MACHINE.MD | Review assumptions leave final course mapping unresolved; A/B/C/D is an assignment example; supplied samples A/Z do not prove final commands |
| Review trials and timing targets mistaken for implementation | T01–T16 remain planned; Safety 10 ms, control/sensing 20 ms, initial age 250 ms and commanded brake ≤50 ms are design targets requiring measurements |
| Current follower presented as full mission | Explicitly limited to right IR + M1/M2; no encoder speed, barcode, terrain/obstacle mission or RTOS tasks |

## Unresolved source issues

The review repeats NFR1/NFR2 as NFR12/NFR13. Preserve the source numbering and avoid treating duplicates as additional independent requirements. Its status example `{"request":"status"}` is illustrative; the implemented command envelope uses session, ID, issued uptime and command. Final mission schema, barcode mapping/finish rule, measured geometry, electrical/timing/resource limits and calibration-dependent pass thresholds require agreement/evidence.

Submitted design authority does not overwrite later explicitly requested bench wiring or turn proposals into tested implementation. Datasheet electrical/ADC facts retain their physical authority. No Word or firmware changes were made by this documentation audit.
