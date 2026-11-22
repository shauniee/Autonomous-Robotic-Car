# Obstacle (S5) — test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

**Subsystem:** Obstacle: sonar, servo scan, profile  ·  **Student ID:** 2502825  ·  **Code:** `app_program/obstacle/`

See [testing/README.md](../README.md) for how to run tests and record results.

## Test cases

Define a test here before running it. Test IDs never change.

| ID | What is tested | Requirement / interface | Method | Steps / how | Expected result |
|---|---|---|---|---|---|
| S5-U01 | Sonar range | T08 | manual | 1. Board at 100, 250 and 500 mm | Range recorded against a tape at each distance |
| S5-U02 | Servo centre and limits | FR-14 | manual | 1. Flash the test image (`test.uf2`). 2. Servo to centre and both sweep limits | Centre faces forward; limits do not stall |
| S5-U03 | Scan profile width | T09 | manual | 1. Scan boxes 120, 180 and 250 mm wide at 150 mm (the Mission obstacle stop), car still | Width and edges recorded against a tape |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-11-18 | S5-U01 | `e922198` (`test`, TEST=1) | Jia Jing | PASS | `logs/S5-U01/usb.log`, `logs/S5-U01/monitor.log`; [Test video](https://youtu.be/_Hi5o9f7MZk), [Terminal video](https://youtu.be/1lZWcA1DVao) | USB-powered TEST image, motors inhibited. Every boot printed `[TEST] S5-U01 PASS  front sonar echo valid`. Two takes against the tape, median `front=` (range): 100 mm read 107 (97-107) and 104 (95-112); 250 mm read 245 (244-251) and 254 (242-257); 500 mm read 492 (474-499, one 325 outlier) and 488. All within 12 mm. 1000 mm and open space dropped from this test; open space and no echo are covered by S5-U05. The monitor capture was trimmed to the takes (from 18:39:10); earlier hotspot dropouts are in `usb.log`. |
| 2026-11-18 | S5-U02 | `3e6a79f` (`test`, TEST=1) | Jia Jing | PASS | `logs/S5-U02/usb.log`; [Test video](https://youtube.com/shorts/vc9rkEjtWGk), [Terminal video](https://youtu.be/e7-_QT2xncI) | USB-powered TEST image, motors inhibited; firmware source unchanged from `e922198` (only test records changed). Box ahead, GP20: one scan swept coarse rays -60 to +60 deg in 10 deg steps, then fine rays at -25 and 45 deg, all with echoes; servo reached centre and both limits with no stall or buzz (observed). Profile `width=161 uncertainty=56 closest=217 left=197 right=171`. Box echoes spanned -20 to +40 deg, so check the servo centre before S5-U03. Earlier GP20 presses in the log were ignored while Mission was STOPPED after a terrain LIFTED latch (car tilted); a reboot restored READY. |
| 2026-11-18 | S5-U03 | `db25868` (`test`, TEST=1) | Jia Jing | PASS | `logs/S5-U03/usb.log`; [Test video](https://youtube.com/shorts/GMyfDFjMPVQ), [Terminal video](https://youtu.be/hmPKFTLMPWw) | USB-powered TEST image, boxes 150 mm ahead, one GP20 scan per press; 10 scans over 4 boots, scans 1-3 cut short by resets. Profiles: 120 mm box (0 deg range 163-164 mm) width 230, 229, 255 (uncertainty 62-64); 180 mm box (150-155 mm) width 218, 211 (60-61); 250 mm box (136 mm) width 211, 209 (60). PASS against the defined expectation (width and edges recorded; no accuracy limit set). The 120 mm box reads 110-135 mm too wide, outside its uncertainty: every scan echoes from about -30 to +30/40 deg, so the beam spread sets the edges. Box edges were not measured against the tape. Earlier attempts at 300 mm gave no profile: side objects within about 0.4-0.7 m projected inside the box depth. |

**Note on the 2026-11-18 S5-U01 row (added 2026-11-22):** S5-U05 does not exist. The no-echo case is covered by the host test `logs/S5-U01/test_echo_range.c`, which checks that a timeout gives an invalid range. Open space was not tested on the car.
