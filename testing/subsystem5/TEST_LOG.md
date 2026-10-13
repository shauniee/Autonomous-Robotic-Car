# S5 test log

Owner: Jia Jing / obstacle. Stationary serial range bench implemented; hardware validation pending. Retired template definitions: S5-U01 task-context example; S5-U02 unfilled manual placeholder. No template result was recorded. No subsystem hardware test is claimed.

## Test cases

### S5-U03 — Stationary USB serial range and timeout checks

Status: NOT RUN. Use USB-only TEST firmware; motors inhibited. Confirm reported
HC-SR04P marking/supply and Echo voltage, GP3 Trig, GP2 Echo, common ground,
and no competing Maker-port device before powering. Leave servo stationary.

1. Open USB serial at 115200 baud. Confirm recurring `[S5]` output.
2. Place a broad flat target perpendicular to the sensor at measured 10, 20
   and 50 cm from its front face. Save at least ten readings per position,
   including pulse duration, measured reference and observed error/spread.
3. Remove the target. Record readings or explicit unknown/timeout messages;
   removal alone does not guarantee no echo because surrounding surfaces reflect.
4. With power disconnected, disconnect Echo from the sensor and hold the Pico
   Echo input LOW through a suitable resistor. Restore USB power and confirm
   recurring `no Echo rise` messages without startup failure. Power off before
   restoring wiring.

Expected: recurring distance/pulse pairs for reflected targets; nominal
`distance_cm = echo_us / 58`; timeouts are unknown, never a clearance value.
Acceptance tolerance is pending measurement/agreement; do not claim calibrated
accuracy. Record revision, TEST profile, UF2 checksum, actual wiring/power,
operator, reference distances, console evidence and failures before PASS.

## Results

No project results recorded.
