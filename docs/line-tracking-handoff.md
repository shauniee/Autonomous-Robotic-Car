# Line-tracking handoff (2026-11-01)

## Contents

- [Where it stands](#where-it-stands)
- [What changed this session](#what-changed-this-session)
- [Open issues, in priority order](#open-issues-in-priority-order)
- [How to run a trial](#how-to-run-a-trial)
- [Lessons from the trials](#lessons-from-the-trials)

State of S3/Mission line tracking at the end of the INT-98 to INT-123 session.
Rules live in the [DOUT navigation contract](architecture.md#dout-navigation-contract),
values in [DOUT navigation commissioning](project-specs.md#dout-navigation-commissioning),
evidence in the [test log](../testing/integration/TEST_LOG.md). This page only says
where things stand and what to do next.

## Where it stands

- Source defaults run 100 mm/s (user-fixed speed) and build a normal image
  byte-identical to trial o (INT-123).
- At 100 mm/s straights, the three 90 degree curves and the plain crossing hold
  (INT-118, INT-123). 55 mm/s passed the course with earlier tuning (INT-105, INT-108).
- Fails: the junction reached at the end of the three-curve route (INT-123).
- Hardware: two DOUT IR modules 30 mm apart, outside both 18 mm line edges, pair
  centre 120 mm ahead of the axle after an 18 mm move back; barcode sensor moved
  with it (about 185 mm forward, 55 mm left). Effective spot spacing 33 mm and line
  width 23 mm were measured before the move (INT-98).
- No physical pass exists for barcode turns (`NAV_GEOMETRY_RELEASED` is still 0),
  obstacle bypass or repeat runs across battery levels.

## What changed this session

- Wheels: feedforward was about 7x too large (70-100% speed overshoot after
  every target change). Fitted `MOTION_KF` 0.0001, `MOTION_KS` 0.05, `MOTION_KP`
  0.0001 (INT-99 to INT-103). Both wheels match within 0.5 duty points.
- Steering: PI on the digital error instead of build/fade, with a slow opposing
  drain, a curve-exit drain and a zero hold (INT-107 to INT-122).
- Curve vs crossing: a large held integral at broad-black entry means curve skew,
  so no crossing commits; a straight crossing steers back to the heading saved
  before the approach (INT-104, INT-108).
- IR read every 10 ms (INT-115); `MISSION_TRACK_SPEED_MMPS` and the `LINE_*`
  steering values can be overridden at build time (host tests use this).

## Open issues, in priority order

1. **Junction after the three curves (INT-123).** Not yet traced. Likely the
   curve-skew rule: if the held integral is still above 32 when both sensors read
   black, the junction is treated as curve skew and driven through with the turn
   held. Check the integral at that broad-black entry first.
2. **No IR state in the traces.** WHEEL records carry wheel targets only, so every
   steering rule above was inferred. Adding IR bits and the integral to the
   15-byte wheel trace (frame owned by
   [architecture](architecture.md#wheel-trace-15-bytes)) would end the guessing.
   Needs owner agreement.
3. **Hump detector stuck on (INT-122).** Mission halved speed to 50 mm/s for a
   whole run while terrain reported a hump; fixed by an IMU check and restart,
   cause not investigated in S4 code.
4. **S-bend with the defaults.** The S-bend failed with hold-until-white (INT-120);
   the 64 kick during the hold (trial o) was not reported separately for an S-bend.
5. **Fixed values to measure, not guess:** tightest curve radius, 20-sample
   overshoot window, effective track width, IR spot spacing at the new 120 mm
   position, left-sensor span and hysteresis.

## How to run a trial

1. Change one value in `app_program/common/robot_config.h`.
2. Build the normal wireless image:
   `docker compose run --rm --user "$(id -u):$(id -g)" firmware make -C build_make -j8 SMP=1 CONSOLE=usb_cdc WIFI=cyw43 NET=lwip MQTT=1 CRASH_REPORT=0 TEST=0`
3. Copy `build_make/mtk3pico_smp1_usb_cdc_wifi_lwip_mqtt.uf2` to a named file,
   restore the source, and record the UF2 hash with the result.
4. Run on battery with `mqtt_client.py monitor` saving to a file; analyse WHEEL
   targets (steering = target ratio, see INT-106 onwards).
5. Run `testing/pure/run.sh` before any default changes; append a new INT entry.

## Lessons from the trials

- Fix the wheels before tuning steering: oversized feedforward looked like
  "steering too strong" (INT-99).
- With two on/off sensors, faster integral build winds up (INT-112) and curve
  speed caps were rejected by the user (INT-111); reducing delay (10 ms IR) and
  moving the sensors closer to the axle were what helped at 100 mm/s.
- Each exit/hold rule fixed one case and moved the problem (INT-119 to INT-122):
  time-based rules cannot cleanly tell a curve ending from an S-bend.
