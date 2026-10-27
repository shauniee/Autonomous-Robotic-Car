# ADR 0001: Base-2 scaled integers for S3 fractions and signed ratios

Status: accepted for S3 (spec #100, ticket #105). Recommended, not imposed, for S1, S2, S4 and S5.

## Context

The minimum-bit requirement ([project-specs §4](../project-specs.md#4-change-in-requirement-minimum-bit-data-representation)) asks every in-scope field to use only the bits its range needs. S3 stored IR fractions (0..1), line error and steering (-1..+1) as 32-bit `float`. The RP2040 Cortex-M0+ has no FPU, so `float` also costs software arithmetic.

## Decision

- A 0..1 fraction is stored as `uint8_t` 0..127 (`IR_SCALE_ONE`); a -1..+1 value as `int8_t` -127..+127. 127 = 2^7 - 1, so a fraction uses 7 bits and a signed value 8.
- Rounding is to nearest. Threshold decisions (line state) compare the unrounded ratio exactly in integer arithmetic, so rounding never changes a line state.
- Float tunables in `robot_config.h` stay `float` and are folded to integers at compile time.
- Firmware sends scaled values unchanged; base-10 display happens only on the laptop (`scripts/mqtt_client.py`).
- The S3 → S2 boundary converts steering to `float` exactly once, in Mission, because `MotionCommand` (S2) takes `float`. An `int8_t` steering field there is proposed to the S2 owner; S2 code is not edited by S3.
- A finer step is adopted only where a test shows 1/127 is not enough (depends on D07 gains).

## Consequences

- Steering resolution is 1/127 (about 0.8 percent). The host replay test `testing/pure/test_ir_tracking.c` shows identical line states and steering within one step of the float reference for the current `IR_KP` / `IR_MAX_CORRECTION`; a gain ratio above 1 widens that bound and must re-run the test.
- Readers divide by `IR_SCALE_ONE` only at a float boundary.
