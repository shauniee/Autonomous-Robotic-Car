# Test catalogue coverage matrix

This matrix maps the requested subsystem families (93 total) and the integration/end-to-end cases to current test IDs and observed evidence. `Host PASS` means the exact existing `testing/subsystemN/subsystemN_test.c` fixture ran on the host against real deterministic production functions using a report-only `tm_printf` adapter; it does not exercise T-Kernel, concurrency, I/O, firmware, or hardware. `Target compile` means the target object compiled, not executed. `PENDING` means the described real target/physical scenario has not been run or its acceptance oracle is still unapproved.

The combined fixture run is at unit-branch revision `81f4714`; output is [here](pure/logs/2026-10-15_fixture-suite.txt), run with `sh testing/pure/run_fixtures.sh`. Subsystem result counts are S1 10/10, S2 15/15, S3 15/15, S4 14/14, S5 5/5. New target fixture IDs were also compiled as part of the `TEST=1` target build; the final firmware link fails for the baseline Arm GNU 15 newlib syscall/linker issue, so no new firmware image or Pico run is claimed. Existing historic Pico captures remain tied only to their recorded old revisions.

## Subsystem 1 (11 families)

| Requested family | IDs | Evidence and remaining limit |
| --- | --- | --- |
| Seven telemetry/status/history topics | S1-U11 | Host PASS; all seven reject wildcards and duplicate names. Broker subscription/reconnect still needs live integration. |
| Mapping request syntax | S1-U09, S1-U14 | Host PASS; valid fragmented request and malformed/unknown fields exercised. |
| Mapping semantics | S1-U09, S3-U20, IT06 | Host PASS for parser/service enum; IT06 target-compiled API check only. Atomic live application/ACK remains IT06 PENDING. |
| Request IDs | S1-U14, IT06 | Host PASS for zero, leading zero, overflow, duplicate JSON key, max ID. Cross-request replay identity only target-compiled IT06; no Pico execution. |
| Fragment assembly | S1-U06, S1-U09, S1-U14 | Host PASS; exact length and split chunks covered. Broker loss mid-fragment is IT06 live protocol PENDING. |
| ACK queue/backpressure | S1-U10 | Host PASS; retry/reconnect generations, stale callback and full queue. Actual MQTT ACK delivery/reconnect remains PENDING. |
| Status/event request parser | S1-U06, S1-U13 | Host PASS; read-only status/events and malformed/movement requests. |
| Serialization | S1-U04, S1-U07, S1-U12 | Host PASS; capacities, nulls, maximum counters, history and cursors. |
| Freshness | S1-U15 | Host PASS at line/motion/terrain/magnetic/front boundaries and wrap. Independent producer task publication remains IT07 PENDING. |
| Event history | S1-U12 | Host PASS for cursor, eviction, saturation and reset. Cross-producer coherent snapshot remains IT07 PENDING. |
| Reconnect | S1-U10, IT06 | Host PASS only for ACK queue reconnect/retry model. Broker reconnect, exactly-once application and retained-state behavior remain IT06/E2E03 PENDING. |

## Subsystem 2 (15 families)

| Requested family | IDs | Evidence and remaining limit |
| --- | --- | --- |
| Quadrature transitions | S2-U06 | Host PASS for direction and illegal-transition table; production `drv_encoder_transition` function extracted verbatim. GPIO IRQ/counter lifecycle is not covered. |
| Encoder lifecycle | S2-U06, IT02, IT03 | IT02/03 procedures PENDING; no IRQ capture, reset/rearm, electrical noise or physical encoder test. |
| Mode validation | S2-U19 | Host PASS for unknown mode and bounds. |
| Request freshness | S2-U11, S2-U15 | Host PASS for lease wrap/exact expiry and sampled-age 60 ms plus stale/future rejection. |
| Command identity | S2-U08, S2-U16 | Host PASS for old epoch, replay, mutation and renewal identity. |
| Lease renewal | S2-U07, S2-U16 | Host PASS for origin/deadline preservation and exact identity. |
| Signed wheel targets | S2-U12 | Host PASS for left/right/U-turn signed encoder travel. |
| Per-wheel completion | S2-U07 | Host PASS; first wheel does not complete action, both required. |
| PID | S2-U13 | Host PASS for independent signed wheel outputs and duty bounds. |
| Invalid feedback | S2-U14, S2-U20 | Host PASS for NaN feedback/speed, missing config and invalid interval with zero output. |
| Failure/stall behavior | S2-U10, S2-U17 | Host PASS for expiry/stall braking and failure reason. Physical stopping budget remains IT03/E2E02 PENDING. |
| Done acknowledgement | S2-U07, S2-U17 | Host PASS for matching completion ACK and one-time return to idle. Stale-ID mission transition remains IT02 PENDING. |
| Reset/epoch | S2-U08, IT03 | Host PASS for stale run epoch; physical stop/reset lifecycle remains IT03 PENDING. |
| Stop priority | IT03 | PENDING: GP21 during motion/start/reset, latch/release race and brake latency require on-target run. |
| Bench gates | S2-U09, S2-U18, S2-U19 | Host PASS for explicit electrical confirmation, bounded duty and deadline. Actual powered commissioning remains PENDING. |

## Subsystem 3 (37 families)

| Requested family | IDs | Evidence and remaining limit |
| --- | --- | --- |
| Paired IR channels/IDs | S3-U18, S3-U22 | Host PASS for paired sample normalization/capture; physical sensor channel identity remains T01 PENDING. |
| Capture duration/count/contrast | S3-U18, S3-U19 | Host PASS for sample count, repeated sample suppression, duration/gap and contrast. Physical optical reference remains PENDING. |
| Stale samples and gaps | S3-U06, S3-U12, S3-U19 | Host PASS for freshness/wrap/gap; task acquisition cadence remains hardware pending. |
| Polarity, normalization, hysteresis | S3-U18, S3-U22 | Host PASS for opposite polarity, endpoints/midpoint and exact enter/exit behavior. |
| Line classifications | S3-U22 | Host PASS for visible/lost/broad-black/unknown. |
| Invalid guidance | S3-U06, S3-U22 | Host PASS for stale/invalid input and zero steering. |
| Calibration lifecycle | S3-U17, S3-U18, S3-U19 | Host PASS for admission, capture, invalidation, timeout and recovery. GP21 behavior outside calibration remains IT03 PENDING. |
| Joint request once | S3-U14 | Host PASS for result/beep precedence helper. Live joint request/task coordination remains T01 PENDING. |
| No readiness from black-only calibration | IT01, S3-U14 | IT01 checks pre-producer initialized snapshots only; actual calibrated readiness transition remains T01 PENDING. |
| Cancellation/deadline/beep/white contrast | S3-U14, S3-U17, S3-U19 | Host PASS for helper/capture behavior; physical button/task timing remains T01 PENDING. |
| A–Z decode, directions and scales | S3-U09, S3-U15 | Host PASS for A–Z at both scales and orientations. |
| Width, quiet zones and gaps | S3-U09, S3-U16, S3-U19 | Host PASS for decoder vectors and service capture; physical tape/encoder tolerances remain T04/T11 PENDING. |
| Malformed/noisy/overflow reads | S3-U16, S3-U23 | Host PASS for malformed frame, ambiguous width and capture queue bounds. |
| Reverse/encoder epoch/deadline | S3-U09, S3-U25 | Host PASS for reverse decode, epoch rejection and frame timeout. Physical encoder IRQ/reset remains PENDING. |
| Unmapped symbol does not navigate | S3-U24 | Host PASS: recorded observation, no action queue entry. |
| Duplicate rearm | S3-U16 | Host PASS for single queued event in one capture and retained history; a second complete same-marker rearm cycle remains T04 PENDING. |
| Capture versus run-history reset | S3-U16 | Host PASS for event retained on scanner reset and run-epoch history setup; full mission reset semantics remain T11 PENDING. |
| Queue/ring bounds | S3-U16 | Host PASS for one-shot event queue/history bounds. |
| Atomic mapping acceptance | S3-U20, IT06 | Host PASS for service mapping validation; IT06 checks real mission submit acceptance/replay/busy but only target-compiled. Atomic live mapping-to-ACK behavior remains IT06 PENDING. |
| Invalid mapping | S3-U20, IT06 | Host PASS for invalid enum/uniqueness helpers; API rejection is target-compiled only. |
| Busy mapping | S3-U21, IT06 | Host PASS for stopped predicate; real pending/busy API test target-compiled only. |
| Distinct-fresh dwell | S3-U21 | Host PASS for sequence/time dwell helper. |
| Qualified junction | S3-U21 | Host PASS for distance/hold/course guard only; line-course traversal remains IT02/E2E01 PENDING. |
| Four action dispatches | IT02, E2E01 | PENDING: manual protocol specifies all four actions; no task-to-motor sequence executed. |
| Ambiguity stops safely | T11, IT02 | PENDING: no explicit executable ambiguity/stop-latch test added; record as an open requirement. |
| Finish conditions | S3-U21 | Host PASS for guard thresholds; route completion remains E2E01 PENDING. |
| Bounded line search/current-run direction | T11 | PENDING: current-run direction/reset and search limit require task/route test. |
| Preserve qualified instruction/second unresolved stop | T11 | PENDING: requires task-level capture-loss and second-marker sequence. |
| Reset clears steering | S3-U22 | Host PASS for invalid guidance returning zero; reset lifecycle/previous steering state remains T11 PENDING. |
| Resume corridor/heading/IR | IT04, E2E01 | PENDING: physical recovery route and reacquisition. |
| Obstacle cancels navigation | IT04 | PENDING: manual obstacle/scan-to-mission handoff sequence. |
| GP21 in every state | S3-U17, IT03 | Host PASS only for calibration cancellation; mission state coverage PENDING in IT03. |
| Release does not unlatch | IT03 | PENDING: requires on-target reset/release trial. |
| Reset/config/start separation | IT03 | PENDING: requires on-target button/state trial. |
| Watchdog/fault | S3-U21, IT03, IT05 | Host PASS for selected health/fault guards; task watchdog and physical braked fault path PENDING. |
| GP21 start/reset race | IT03 | PENDING: task/interrupt race cannot be established by pure guards. |
| IR-only profile does not enable mission | INT-27, IT01 | Prior profile audit exists; current IT01 target object compiled only and checks initialized snapshots, not mission task execution. Runtime profile acceptance remains PENDING. |

## Subsystem 4 (15 families)

| Requested family | IDs | Evidence and remaining limit |
| --- | --- | --- |
| Sensor conversions/calibration | S4-U11 | Host PASS for configured sign-to-angle/gravity conversion. Independent physical axis permutation/reference is not measured. |
| Accelerometer/magnetometer validity independent | S1-U15, IT07 | Host PASS for telemetry’s independent stale/null fields; task-level independent sensor acquisition validity remains IT07/T06 PENDING. |
| Unique samples/100 samples/gravity | S4-U05, S4-U12 | Host PASS for 100 distinct samples, duplicate suppression and gravity range. |
| Stationarity and request identity | S4-U12 | Host PASS for matching request, motion/stationarity and recovery checks. Live task identity remains T01 PENDING. |
| Abort | S4-U06, S4-U12 | Host PASS for movement, stale acquisition, deadline and cancellation; live task abort remains T01 PENDING. |
| Recovery | S4-U10, S4-U12 | Host PASS for fresh request/reinitialization helper behavior; physical reset/recalibration remains T01/T14 PENDING. |
| Derived dt/sequence/epoch | S4-U13 | Host PASS for yaw wrap, duplicate/zero dt and cross-epoch rejection. |
| Classification priority | S4-U14 | Host PASS for impact/stationary/turn/terrain/acceleration precedence. |
| Validity flags | S4-U03, S4-U08, S4-U15 | Host PASS for invalid windows and current/max retention. Producer snapshot coherence remains IT07 PENDING. |
| Hump phases | S4-U08, S4-U15 | Host PASS for climb/crest/descent/baseline and roll-only profile. Physical peak accuracy remains T06 PENDING. |
| Pitch/roll height | S4-U11, S4-U15 | Host PASS for pure derived values; physical reference measurement remains T06 PENDING. |
| Reject bad travel | S4-U15 | Host PASS for negative travel invalidation. |
| Accepted maximum/history | S4-U03, S4-U08, S4-U09, S4-U10, S4-U15, S4-U16 | Host PASS for retaining accepted values, history eviction/overflow and reset rules. |
| Stop/run reset | S4-U04, S4-U09, S4-U10, S4-U16 | Host PASS for pure snapshot/context clearing and retained results. Stop/reset task integration remains T07/T14 PENDING. |
| Coherent snapshots | S4-U04, S4-U16, IT07 | Host PASS for sequential snapshot helpers; concurrent publish/telemetry coherence remains IT07 PENDING. |

## Subsystem 5 (15 families)

| Requested family | IDs | Evidence and remaining limit |
| --- | --- | --- |
| Echo timing | S5-U07 | Profile range checks only; GPIO edge timing is PENDING. |
| Invalid Echo | S5-U04, S5-U07 | Host PASS for invalid/missing rays at profile boundary; electrical Echo pulse classes PENDING. |
| Trigger output | S5-U07 | No GPIO trigger pulse execution; PENDING. |
| Servo configuration | S5-U07 | Profile-angle bounds are tested, not servo PWM calibration; PENDING. |
| Scan admission | S5-U05, S5-U08 | Host PASS for pure cancellation/failure guards; real stationarity/request admission PENDING. |
| Coarse/fine phases | S5-U07 | No servo/scan task phase sequence; PENDING. |
| Bad rays | S5-U04, S5-U07 | Host PASS for profile rejection of insufficient/invalid rays; raw Echo IRQ sequence PENDING. |
| Forward invalid during scan | S5-U05, S5-U08 | Pure guard truth table only; concurrent sensor-health transition PENDING. |
| Cancel | S5-U05, S5-U08 | Host PASS for cancellation precedence; physical cancellation during each scan/restore phase PENDING. |
| Recenter/fresh completion | S5-U05 | Pure guard only; center restoration and fresh completion sample PENDING. |
| Profile validation | S5-U04, S5-U06, S5-U07 | Host PASS for ray count, bounds and geometry. |
| Geometry | S5-U06 | Host PASS for forward mounting offset; course clearance/reacquisition PENDING. |
| Boundary profiles | S5-U07 | Host PASS for profile angular boundary and invalid ranges. |
| Unknown gaps | IT04, E2E01 | PENDING: unknown/occluded clearance must remain stopped; manual oracle supplied. |
| Lifecycle | S5-U05, S5-U08 | Host PASS for guard conditions; task request IDs, IRQ timing, scan/cancel lifecycle and servo restoration PENDING. |

## Integration cases (7)

| ID | Scope | Result |
| --- | --- | --- |
| IT01 | S2/S3/S4 initialized snapshot/readiness guards | Target-compiled only. Checks pre-producer initialized states and null outputs; it does not prove joint calibration or READY transition. Live calibration/readiness remains T01 PENDING. |
| IT02 | S3/S2 four barcode actions through junction, motion, encoder done, matching ACK and next navigation; stale action ID rejection | Manual procedure with explicit oracle; PENDING. Stale-ID injector availability and approved distance/angle tolerances unresolved. |
| IT03 | S3/S2 GP21 during motion/start/reset; latched stop, no release race | Manual procedure; PENDING. Stop-time limit must be approved/measured. |
| IT04 | S5/S2/S3 obstacle detect/brake/stationary scan/profile/bypass/reacquire; cancellation/health/unknown clearance | Manual procedure; PENDING. Course profile and clearance limits require approval/measurement. |
| IT05 | S4/S3/S2 impact fault-stop; discard partial hump, retain prior accepted history | Manual procedure; PENDING. Impact and stopping references require approval/measurement. |
| IT06 | S1/S3 stopped-only mapping submission, replay/fragments/broker loss and ACK | Real `mission_mapping_submit` API case target-compiled only; leaves one volatile valid mapping pending in the inhibited TEST boot. Manual live application/ACK/reconnect steps remain PENDING; reboot/reflash before normal-image acceptance. |
| IT07 | All producers/S1 coherent snapshot, telemetry/history and autonomy despite blocked comms | Sequential APIs cannot prove concurrent publication. Exact live procedure supplied; PENDING. |

## End-to-end cases (3)

| ID | Scope | Result |
| --- | --- | --- |
| E2E01 | Boot/calibrate/configure four mappings on unchanged firmware/start/line/four actions/humps/obstacle bypass and finish | Manual full-route procedure; PENDING. Route, calibration, tolerances and peak references must be recorded. |
| E2E02 | Separate GP21, stale sensor, line-loss deadline, scan timeout/unusable-clearance faults; latched safe stop/restart refusal/reset | Manual fault matrix; PENDING. Approved stop-time and clearance limits are unresolved. |
| E2E03 | Broker disconnect/reconnect and post-run power cycle; autonomy, telemetry/history restore, reboot map/readiness clear and separate start | Manual recovery procedure; PENDING. No live broker/power-cycle execution. |
