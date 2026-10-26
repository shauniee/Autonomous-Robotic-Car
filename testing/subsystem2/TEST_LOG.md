# S2 Motion test log

Owner Mahesha / 2503973. Template example S2-U01/U02 are retired, not reused. Retired definitions: S2-U01 example values in 0..100; S2-U02 pool/slot bounds. No template result was recorded.

## Contents

- [Test cases](#test-cases)
- [Results](#results)

## Test cases

| ID | Test / requirement | Method / steps | Expected |
|---|---|---|---|
| S2-U03 | Freshness, stop gate; IF-01/02 | On-target TEST=1 self-check | `[TEST] S2-U03 PASS`; outputs remain inhibited |
| S2-U04 | Duty bounds, expiry, uint32 wrap; [Motion inhibition contract](../../docs/architecture/MOTION.md#business-logic) | Historical host policy check; runner retired | Invalid/expired/stopped request rejected |
| S2-U05 | Real Motion service / kernel contract | Historical real-service host check, normal and TEST policies; runner retired | Boot latch, reset invalidation, expiry latch, independent stop on mutex failure; TEST always brakes |

## Results

| Date | ID | Source | Tester | Result | Evidence | Notes |
|---|---|---|---|---|---|---|
| 2026-10-08 | S2-U04 | Manifest in integration/logs | Codex host run | PASS | ../integration/logs/2026-10-08_host-checks.txt | Logic only |
| 2026-10-08 | S2-U05 | Manifest in integration/logs | Codex host run | PASS | ../integration/logs/2026-10-08_host-checks.txt | Real service with fake kernel/output, both profiles |
| 2026-10-08 | S2-U03 | Not yet run on Pico | Pending operator | PENDING | none | Built only; no on-target pass |
