# System — test log

## Contents

- [Test cases](#test-cases)
- [Results](#results)

Tests of the complete robot against its requirements, on the final hardware set-up. Ready-to-run checks: [mission readiness](mission-readiness.md).

See [testing/README.md](../README.md) for how to run tests and record results.

## Test cases

Define a test here before running it. Test IDs never change.

No system test is kept. SYS-01, SYS-02, SYS-05, SYS-07 and SYS-10 were dropped first; SYS-03, SYS-04, SYS-06, SYS-08 and SYS-09 on 2026-11-20: the integration tests from the [full course run](README.md#one-full-course-run) cover the demo, the local stop is shown by S4-U04 (lift stop), and the network has no drive path to test. Their IDs are not reused.

| ID | What is tested | Requirement / interface | Method | Steps / how | Expected result |
|---|---|---|---|---|---|

*Method:* **auto** = `TEST_CHECK` in code, result printed on the console;
**manual** = a person observes the result (motor, sensor, dashboard, tape measure).
Each test is shown in one [video](../../video/README.md).

## Results

Add a new row for every run. Never edit or delete old rows.

| Date | Test ID | Commit | Tester | Result | Log / evidence | Notes |
|---|---|---|---|---|---|---|
| YYYY-MM-DD | SYS-nn | `abc1234` | | PASS / FAIL | `logs/<ID>/<what>.log`, video link | |
