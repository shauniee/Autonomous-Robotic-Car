# Software restructure verification — 2026-10-29

## Contents

- [Scope](#scope)
- [Sources and results](#sources-and-results)
- [Limits](#limits)

## Scope

Software-only verification of the initial three-task restructure. No board was
flashed or moved. The kernel-free suite uses actual policy/codec/ring/algorithm
modules; it does not fake a kernel or establish on-target timing. Builds use
Arm GNU 13.2.Rel1, Pico SDK 2.2.0, SMP=1, USB CDC, CYW43 and CRASH_REPORT=0,
with guards enabled. Each source built NET=none/MQTT=0 and NET=lwip/MQTT=1,
with TEST=0 and TEST=1. Existing documentation drafts were excluded from source
commits. Case definitions and appended results are in the integration TEST_LOG;
the immutable manifests and logs below identify each tested software source.

## Sources and results

| Case | Source commit | Recorded evidence / result |
| --- | --- | --- |
| INT-12 | `fe39e3a5a9ef37a3a1205e7470e89e9e3a19dc30` | [Manifest](logs/2026-10-29_INT-12_stage1-manifest.json), host; host and four builds PASS |
| INT-13 | `b862817a40b7b75c7d2524621a1fffb5949a8f0f` | [Manifest](logs/2026-10-29_INT-13_stage2-manifest.json), host; host and four builds PASS |
| INT-14 | `ab54479a1b0044871cd6a6132c1747d6b3628299` | [Manifest](logs/2026-10-29_INT-14_stage3-manifest.json), host; host, 100000 concurrent mapping exchanges and four builds PASS |
| INT-15 / S2-U06 | `407cbfa362a91db81706b255664a9425be0c33f6` | [Manifest](logs/2026-10-29_INT-15_stage4-manifest.json), host, [strict compile](logs/2026-10-29_INT-15_stage4-strict-app.txt), [allocation/image audit](logs/2026-10-29_INT-15_stage4-audit.json); host, 33 strict application compilations and four builds PASS |

The final source audit finds three task creation sites, five application mutexes,
one stop flag and no foreign motor/network calls. Linked task symbols are
Motion, Mission and I/O; no retired task entry remains. Requested application
stack allocation is 6144 bytes. Wireless normal linked text/data/BSS are
368840/392/84040 bytes; these are linked section sizes, not runtime headroom.
Built-image checksums are recorded in the final manifest; binaries are excluded.
The upstream lwIP unused-function warning remains in wireless builds; application
compilation passes with `-Werror`. The final source's wheel calculations precede
the output exclusion section, and boot braking must service its initial request.

## Limits

Unchanged provisional timing settings are authorized for software development.
Physical calibration, WCET/jitter/ISR load, stack high-water, brake latency and
stopping distance were not measured. The requested moving recovery sweep,
repeated scan filtering/edge/coverage behavior, accepted bypass, Terrain new-run
reset policy and final mixed-mission evidence remain open. A bounded last-side
arc is retained; unknown-side line loss stops. No lost echo proves an obstacle
edge. No full-plan or physical PASS is claimed.

The temporary clean build tree avoids iCloud-generated dependency stalls and
uses the exact workspace sources with copied Make rules. [Exploratory logs](logs/2026-10-29_restructure-exploratory/)
retain setup failures/review attempts, without treating their unmanifested
working trees as accepted sources. All accepted results above include exact
source manifests; original historical evidence was preserved.
