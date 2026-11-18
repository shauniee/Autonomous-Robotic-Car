# System — test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

Tests of the complete robot against its requirements, on the final hardware set-up. Run the full set before every demo and before hand-in. Ready-to-run checks: [mission readiness](mission-readiness.md).

See [testing/README.md](../README.md) for how to run tests and record results.

## Test cases

Define a test here before running it. Test IDs never change.

| ID | What is tested | Requirement / interface | Method | Steps / how | Expected result |
|---|---|---|---|---|---|
| SYS-01 | Electrical readiness | MR-01 to MR-05, T01 | manual | 1. Continuity against the pin table. 2. Rails under load. 3. Sonar echo level | All within the pin table and ratings |
| SYS-02 | TEST image self-checks | MR-06, MR-07 | auto | 1. `./flash.sh test` on USB. 2. Save the console | Every `[TEST]` line PASS; motors never move |
| SYS-03 | Mission-day routine | MR-35 | manual | 1. Fresh boot, broker, unfamiliar letters mapped, local start | Starts and runs with no recompile |
| SYS-04 | Full course | MR-36, T16 | manual | 1. Course with line, barcodes, junctions, a hump and an obstacle. 2. Save `monitor` | Course completed; run time recorded against about five minutes |
| SYS-05 | Three runs in a row | MR-37 | manual | 1. Repeat SYS-04 twice more with no code or constant change | Three passes in a row |
| SYS-06 | Stop and reset | MR-24, A12, T02 | manual | 1. Press GP21 at course speed. 2. Hold GP20. 3. Release and press GP20 fresh | Stops at once; no restart until a fresh GP20; stopping distance taped |
| SYS-07 | Fault handling | MR-15, MR-16, T14 | manual | 1. Wiggle an encoder lead mid-run. 2. Tap the chassis. 3. Unplug the IMU | Each latches a stop and is named on the dashboard |
| SYS-08 | Network never drives | Authority, T13 | manual | 1. Stop WiFi mid-run. 2. Restart it. 3. Send requests while stopped | Robot keeps its local state; network never starts or drives it |
| SYS-09 | Endless operation | MR-40 | manual | 1. Repeated loops over the same junction with and without a queued turn. 2. GP21 ends the session | Every cycle behaves the same |
| SYS-10 | Timing and memory under full load | MR-39, T15 | manual | 1. SYS-04 on `./flash.sh trial` with the USB console saved | `[TIMING]` max under each task period; overruns do not grow; stack and RAM recorded |

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | SYS-01 | `abc1234` | | PASS / FAIL | `logs/<ID>/<what>.log`, video link | |
