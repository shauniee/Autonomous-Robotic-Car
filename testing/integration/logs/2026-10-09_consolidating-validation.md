# Consolidated S3 software validation

Code revision: `48cbb19`. Documentation commits following it do not change runtime source.

- Sixteen stable firmware fixtures executed through a temporary laptop harness: **PASS**, AddressSanitizer + UndefinedBehaviorSanitizer, strict C11 warnings.
- Twenty additional barcode scenarios: **PASS**, A/B/C/D/Z at both scales and polarities, unequal per-wheel encoder spacing, width jitter, varying elapsed time, timestamp/count wrap.
- Nine sequential firmware profiles: **PASS**, GNU C11 ARM build, modes 0/1 × offline/wireless × normal/TEST, plus offline SMP=0 navigation TEST compatibility.
- Optional configuration path: separate synthetic configured-header wireless/navigation compile **PASS** at `cac5fae`. No values are robot calibration. Adding/removing the header invalidates the build profile.
- Actual localhost MQTT collector/SSE/browser: new fields and legacy missing-field fallback **PASS**; no console errors. Stale observations labelled stale. JavaScript syntax and Python compile **PASS**.

No new Pico firmware test was executed. No robot was available. Software results do not establish peripheral timing, quadrature capture fidelity, physical motor/brake behavior, tuning or original-line identity.

## Build and memory capacity

| Profile | Text bytes | Data bytes | BSS bytes |
| --- | ---: | ---: | ---: |
| smp1-nav0-wireless0-test0 | 302788 | 92 | 31660 |
| smp1-nav0-wireless0-test1 | 346716 | 92 | 42436 |
| smp1-nav0-wireless1-test0 | 374180 | 104 | 89724 |
| smp1-nav0-wireless1-test1 | 401740 | 104 | 100500 |
| smp1-nav1-wireless0-test0 | 305972 | 92 | 31880 |
| smp1-nav1-wireless0-test1 | 347676 | 92 | 42656 |
| smp1-nav1-wireless1-test0 | 377356 | 104 | 89944 |
| smp1-nav1-wireless1-test1 | 402700 | 104 | 100720 |
| smp0-nav1-wireless0-test1 | 341316 | 80 | 38536 |

The largest static RAM use is data+BSS, about 101 KB of the linker's 256 KiB region. Firmware links within capacity; remaining RAM also supplies kernel dynamic objects and task stacks. Four application tasks allocate 2048-byte stacks each; startup allocates 1024 bytes normally and 4096 in TEST. Wireless MQTT uses its existing 8192-byte service stack. These allocations fit the static capacity budget, but this is not a measurement of remaining runtime memory.

Compiler stack reports record individual frames, including the TEST startup S1 fixture frame (880 bytes) and coordinator navigation frame (832 bytes). The enlarged JSON buffers in older S1 startup fixtures were moved to static fixture storage so both payload arrays do not occupy the startup stack. Call chains, kernel interrupt frames and runtime high-water still require on-Pico measurement. No ISR WCET or real-time scheduling pass is claimed.

## Reproducibility and evidence boundaries

[Manifest](2026-10-09_consolidating-manifest.json) records the exact code revision, 689 source-file hashes, compiler/SDK details, temporary harness source/checksums and commands. [Final build matrix](2026-10-09_consolidating-acceptance-builds.json) records each command, UF2 checksum and compiler stack frames; adjacent profile logs include compiler output and sizes. [Fixture results](2026-10-09_consolidating-software.txt), [spacing cases](2026-10-09_consolidating-spacing.txt), [dashboard observations](2026-10-09_consolidating-dashboard.txt) and [configured-header build](2026-10-09_consolidating-configured-build.json) preserve their own methods/revisions.

Earlier saved build rounds remain historical evidence and are superseded by the acceptance matrix for final-source claims. The initial wireless compilation failed because the isolated checkout lacked a local Wi-Fi header. Later builds used only the public example copied to an ignored header, then removed; private credentials were not copied or recorded. Final navigation configuration is absent deliberately: opt-in startup refuses movement until measured Motion, barcode and route settings exist.

Pure fixtures exercise decision logic, including stop/reset ordering, action lifetime, expiry and simultaneous line/barcode context. They do not execute the actual kernel scheduler, physical IRQ arrival ordering or motor transport. Permanent firmware wrappers compile into TEST and retain stable IDs; their new on-Pico status is **NOT EXECUTED**. Historical physical/USB results apply only to their recorded revisions.
