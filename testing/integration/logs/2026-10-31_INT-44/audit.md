# Repository audit — 2026-10-31

## Contents

- [Scope](#scope)
- [Findings](#findings)
- [Verification](#verification)
- [Limits](#limits)

## Scope

Current working tree on `fix/wifi-startup-diagnostics`, based on
`9bcab80f4ae299694eff7b2564600883fee0891d`, including existing uncommitted work.
No production files edited, committed or pushed. New evidence is under INT-44;
the integration log receives an appended result.

Inventory covered firmware, five subsystems, Mission/Safety, shared state,
active RP2040 runtime/network integration, laptop tooling/dashboard,
Make/Docker/CI configuration, documentation contracts and testing evidence.
Detailed review concentrated on application behavior and active runtime
boundaries. Imported kernel/SDK and inactive processor ports were not audited
line by line.

## Findings

1. **[P2] Absolute encoder averaging has a half-range discontinuity.**
   `app_program/subsystem2/encoder_math.h:55–57` chooses the shortest modular
   difference between the wheels' lifetime counts. Repeated turns can diverge
   those counts; crossing their half-range separation abruptly changes the
   chosen direction. Actual helper reproduction: `(1000, 33767)` to
   `(1001, 33769)` moves both wheels forward, but mean travel becomes
   **-32766 counts**. Mission junction qualification, Terrain displacement
   and bypass clearance/reacquisition use this helper. Barcode decoding already
   averages per-wheel deltas and is unaffected by this helper. Accumulate mean
   per-wheel increments with consumer-owned origins instead of averaging
   lifetime counts. [Probe](encoder-mean-repro.c), [output](encoder-mean.txt).

2. **[P2] Terrain loses displacement on a pending IMU read.**
   `app_program/subsystem4/subsystem4.c:97–105` advances encoder origins on
   every acquisition attempt, but only a fresh result reaches
   `terrain_algo_process()` at line 166. PENDING inside the freshness window
   drops that interval's travel without invalidating the hump. Ten counts
   before PENDING and ten before the next FRESH contribute only ten counts
   to height integration. The driver explicitly returns PENDING when the
   sensor new-data bit is clear; independently clocked sampling can hit this
   path. Keep an integration origin or accumulated displacement until FRESH,
   with explicit discard/invalidation on a real failure. Source-path finding;
   no physical sensor measurement is claimed.

3. **[P2] Rejected mappings return CLI success.**
   `scripts/mqtt_client.py:386–388` completes on every matching ACK; line 422
   returns zero without checking its result. BUSY, INVALID and MALFORMED look
   successful to calling scripts although no mapping was installed. Preserve
   the result and return nonzero unless it is OK. A test double exercises the
   real `main()` with no broker or robot and reproduces all three cases.
   [Probe](cli-ack-repro.py), [output](cli-ack.txt).

4. **[P2] CI omits both dashboard suites.**
   `.github/workflows/firmware.yml:17–18` runs only `testing/pure/run.sh`, which
   invokes neither `test_dashboard.mjs` nor `test_dashboard_http.py`. Live/offline
   command gates, early-ACK handling and HTTP origin/input checks can regress
   while CI remains green. Add explicit steps for the existing tests. Both
   suites passed when run separately here.

## Verification

- Pure host suite: PASS, including default/guarded Motion, PID/binary Mission,
  mapping concurrency, codecs, sensor helpers and bridge stress. [Output](host.txt).
- Dashboard JavaScript: PASS. [Output](dashboard.txt).
- Loopback HTTP: PASS. [Output](http-retry.txt). The initial sandbox could not
  bind; its [failure output](http.txt) is preserved.
- Four README firmware profiles: PASS compile, link and UF2 conversion.
  Common flags: `SMP=1 CONSOLE=usb_cdc WIFI=cyw43 CRASH_REPORT=0`.
  Offline `NET=none MQTT=0` and wireless `NET=lwip MQTT=1`, each TEST=1/0.
  Sequential builds in an ephemeral copy using existing `robot-build` Docker
  image, placeholder credentials and network disabled. [Output](firmware-builds.txt).
  Only logged warnings: an unused function in upstream lwIP MQTT.
- Encoder and CLI defects: reproduced by saved probes, which assert the
  observed defects rather than claiming correct behavior.
- Tracked-path checks found no committed firmware images, video files or real
  Wi-Fi secret header. Secret contents were not printed or audited.

Source identity: [initial manifest](manifest.json), supplemented by the
[full production source manifest](source-manifest-full.json), including runtime,
linker and build inputs. Base HEAD alone does not identify these uncommitted files.

## Limits

No firmware flashed/executed, hardware motion, real sensor tests, broker
publishes, timing measurements, stack high-water checks or floor trials.
Build sizes are not live memory margins. No fresh visual/browser screenshot
review; dashboard verification used its existing automated suites.

Disabled stall detection is a documented user trial setting, not a new defect.
Physical bypass release remains disabled. Recovery/normal motion still use
provisional geometry, polarity, gains and timing. Pending decisions and the
mission-readiness checklist remain authoritative for physical acceptance.
Documented anonymous plaintext lab MQTT was not treated as an accidental
production exposure. No physical readiness gate is closed by this audit.
