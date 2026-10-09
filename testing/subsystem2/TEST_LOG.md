# S2 Motion test log

Owner Mahesha / 2503973. Template example S2-U01/U02 are retired, not reused. Retired definitions: S2-U01 example values in 0..100; S2-U02 pool/slot bounds. No template result was recorded.

## Test cases

| ID | Test / requirement | Method / steps | Expected |
|---|---|---|---|
| S2-U03 | Freshness, stop gate; IF-01/02 | On-target TEST=1 self-check | `[TEST] S2-U03 PASS`; outputs remain inhibited |
| S2-U04 | Duty bounds, expiry, uint32 wrap; [Motion inhibition contract](../../docs/architecture/MOTION.md#business-logic) | Historical host policy check; runner retired | Invalid/expired/stopped request rejected |
| S2-U05 | Real Motion service / kernel contract | Historical real-service host check, normal and TEST policies; runner retired | Boot latch, reset invalidation, expiry latch, independent stop on mutex failure; TEST always brakes |
| S2-U06 | Forward duty boundary | TEST=1 pure firmware fixture | Zero/0.35 accepted; negative, excessive, NaN and infinite duty rejected |
| S2-U07 | Disabled request and cached acquisition expiry | TEST=1 pure firmware fixture across uint32 clock wrap | Disabled rejected; original sample age 60 accepted and 61 rejected |
| S2-U08 | Stop generations and reset linearization | TEST=1 local state fixture using the real public SMP CAS primitive; no live reset | New stop wins over earlier reset; pre-reset commands cannot replay; stop after reset invalidates its token; generation cannot wrap |
| S2-U09 | Reset guards and null copy | TEST=1 pure/local firmware fixture | Matching stop/brake generation, checked braked output and healthy driver required; exhausted tokens and null destination rejected |
| S2-U10 | Live inhibited Motion service and PWM readback | TEST=1 read-only check of live health and owned PWM registers; up to 60 ms wait | Real service advances, applied duty zero, driver/readback healthy; matching current-stop brake ack; no wheel-motion claim |
| S2-U11 | Physical stop/reset/start acceptance | Battery-only normal profile; record flashed revision, GPIO polarity, requested/applied duty, GP21 press, matching brake ack, release/reset GP20, then separate fresh start | Output brake meets measured 50 ms engineering target; held/simultaneous buttons cannot rearm; reset remains zero and old command does not replay; physical stopping measured separately |
| S2-U12 | Integrated stale request / control failure | Inhibited TEST trial first; interrupt required sensing/control progress with approved bounded fault injection, observe generation/ack; normal raised-wheel trial only after TEST | Requests older than 60 ms stop locally; newer independent stop defeats pending reset; watchdog/reset behavior recorded separately; no telemetry-driven movement |

## Results

| Date | ID | Source | Tester | Result | Evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-08 | S2-U04 | Manifest in integration/logs | Codex host run | PASS | ../integration/logs/2026-10-08_host-checks.txt | Logic only |
| 2026-10-08 | S2-U05 | Manifest in integration/logs | Codex host run | PASS | ../integration/logs/2026-10-08_host-checks.txt | Real service with fake kernel/output, both profiles |
| 2026-10-08 | S2-U03 | Not yet run on Pico | Pending operator | PENDING | none | Built only; no on-target pass |
| 2026-10-09 | S2-U03/U06–U10 | [source hashes](logs/2026-10-09_s2-stop-reset_source-sha256.json) | Codex static compile | BUILT ONLY | [strict compile](logs/2026-10-09_s2-stop-reset_strict-compile.txt) | All S2 C translation units compile with -Wall -Wextra -Werror for SMP 0/1 and TEST 0/1; no linking, flashing or executed test pass |
| 2026-10-09 | S2-U03/U06–U12 | Commit/revision to record for trial | Pending operator | PENDING | none | Firmware self-tests and physical stop/reset/tracking acceptance remain unexecuted |

## Current stop/reset contract

S2 boots at generation 1, stopped. Every accepted `s2_stop()` invalidates the
previous generation independently of the request mutex and wakes Motion. This
accepts a stop request; an already-running PWM transaction may finish before
the service's post-check brakes. Acknowledgement is observed through the
nonblocking `s2_motion_copy()` snapshot. Failed copies leave the destination
unchanged and must not be treated as fresh health.

Only Motion processes `s2_request_reset(expected_generation)`. It discards old
commands, requires its checked brake acknowledgement and healthy driver, then
compare-and-swap clears exactly that stopped generation. A later stop defeats
that reset. Callers wait for matching `reset_ack_generation` and `!stopped`
before issuing a new generation-tagged command. Reset never restores a target.

`brake_ack_generation` with `output_braked` records checked zero PWM writes and
configuration/readback, not wheel stationarity. Requested fractions remain
separate from the quantized software-written applied fractions. TEST always
brakes. Driver/configuration/readback failure inhibits reset until reboot;
clock/wait failure parks the service braked without advancing healthy progress.

No encoder, wheel PID, distance or angle action is introduced by this change.
Historical S2-U04/U05 results predate this implementation and do not establish
its runtime or hardware acceptance. Host runners remain retired.
