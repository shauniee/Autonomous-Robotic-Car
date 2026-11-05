# Saved-capture review

## Contents

- [Source and method](#source-and-method)
- [Findings](#findings)
- [What remains unresolved](#what-remains-unresolved)

## Source and method

Reanalysed existing INT-154, INT-156 and INT-157 gzip captures in
[raw captures](2026-11-02_raw-captures/). Line numbers below are decompressed
lines. Firmware identity remains exactly as recorded in the original test rows;
these captures do not carry an exact clean image/source manifest. No new robot
run, physical measurement or safety PASS occurred. The ten battery-related
restarts are excluded from software fault diagnosis by Nicholas's decision.

The corrected triage tool includes telemetry with `speed n/a`, identifies state
transitions and compares laptop-arrival speed with acquisition-time WHEEL data.
A fault mentioned on many telemetry lines is not that many separate faults.

## Findings

| Capture | Telemetry / WHEEL frames | Highest absolute arrival estimate | Highest absolute acquisition-time WHEEL speed |
| --- | --- | --- | --- |
| INT-154 | 1033 / 27552 | 952 mm/s | 210.32 mm/s, right, sampled_ms 194210, line 21443 |
| INT-156 | 572 / 15358 | 194 mm/s | 330.50 mm/s, right, sampled_ms 122230, line 7825 |
| INT-157 | 639 / 16672 | 1935 mm/s | 180.27 mm/s, right, sampled_ms 71170, line 11578 |

- **INT-157 speed spike:** line 11667 displays L=1935 R=-222 after NAVIGATE.
  None of the saved WHEEL records reaches that magnitude. This supports the
  arrival-timing explanation addressed by PR149; it does not prove encoder
  electrical health. INT-156 does contain a 330.50 mm/s WHEEL sample against a
  123.62 mm/s target, so loaded control/encoder investigation remains necessary.
- **INT-157 final moving segment:** line 12716 is NAVIGATE with five reported
  humps. At sampled_ms 96160–96400, line traces show the programmed STOP command
  inside NAVIGATE, not a STOPPED state. At 97360/97420, both sensors are black and
  TURN is commanded at 100 mm/s. The last nonzero measured WHEEL record is line
  12912 at 97610 ms (targets -100.01/+100.01, measured -85.84/+90.14). The capture
  next shows a fresh READY/unmapped boot. It cannot establish whether the turn
  found the intended branch, nor a steering threshold that would fix the reported
  off-course motion. Do not tune onset skew from this incomplete endpoint alone.
- **INT-156 SCAN→STOPPED:** lines 7966/7971 show FOLLOW and front 687 mm;
  8020/8070 show SCAN, unchanged wheel counts and no echo; 8124 is STOPPED and
  8178 READY. There is no saved scan result or detailed stop reason in this
  interval. A short intervening range observation, scan failure or local stop
  cannot be distinguished from this MQTT evidence. No echo also occurs while
  FOLLOW continues; it is not itself evidence of the stop cause. Existing host
  cases verify report-only no echo and bounded scan/bypass guards.
- **INT-154 calibration:** 80 telemetry lines carry `IMU calibration failed`,
  including a FOLLOW segment beginning at line 22901. This records failed boot
  calibration and report-only continued motion, not its cause. The serial
  `[IMU]` sample/timeout/init messages are needed to distinguish movement,
  insufficient fresh samples and I2C failure.
- **Barcode overflow/events:** capture-overflow faults occur on 429, 127 and 64
  telemetry lines respectively. Those are persistent fault observations, not
  measured ring high-water marks or independent overruns. A retained barcode
  display is not a new decode count. PR149's Mission fallback can preserve a
  barcode notification when a different Mission event occupies the output;
  queue/mutex/network loss still prevents any claim of exactly-once delivery.
- **Humps:** INT-157 increases to five before the final navigation segment,
  matching the reported wrong count. The current stationary-baseline fix is
  newer than this capture. HUMP frames expose filtered tilt and limited travel
  validity, not raw accelerometer samples or the full baseline/window state;
  they cannot faithfully replay the corrected detector or calibrate its error.

## What remains unresolved

Use the existing current-image host regressions for software invariants. At the
next physical run capture USB stop/scan/IMU reasons plus MQTT and video, with
source/image hashes and a declared lap interval. Record actual hump crossings,
reference height/error, barcode passes, encoder health and queue occupancy.
INT-122/133 stuck encounters also lack the raw IMU input and complete detector
state needed to reproduce a physical root cause from these saved captures.
No control gains, geometry, safety masks or detection thresholds were changed
on the basis of these observations.
