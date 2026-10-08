# S1 test log

Owner: Shaun / communications. S1 now owns the optional latest S3 snapshot and bounded encoder, with one mutex and no task. Template demonstration IDs S1-U01/U02/U03 remain retired and are preserved in [upstream history](../../docs/history/upstream-test-logs/subsystem1.md). No Pico hardware result is claimed.

## 1. Test cases

| ID | Scope | Method | Expected |
| --- | --- | --- | --- |
| S1-U04 | Bounded JSON | Host actual encoder; TEST image contains a smaller self-check | Null unavailable values, negative/positive fixed decimals, original timestamp/age including wrap; exact capacity succeeds, undersized/oversized or unterminated input fails without overrun |
| S1-U05 | Latest snapshot and adapter | Host actual S1 store/S3 adapter with fake kernel, plus simultaneous pthread producer/consumer using try-lock | Safe before init and on allocation/busy failure; TMO_POL only; complete paired values, sequence advances on stored update, independent producer storage; requested duties and TEST flag reported without controller changes |

## 2. Results

Historical migration status: no S1 project results were recorded while this subsystem was an inactive placeholder.

| Date | ID | Source | Tester | Result | Evidence | Limits |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-10-08 | S1-U04/U05 | [source manifest](../integration/logs/2026-10-08_s3-mqtt-source-sha256.json) | Codex host run | PASS | [host log](../integration/logs/2026-10-08_s3-mqtt-host-checks.txt) | Real modules with fake mutex/clock; not Pico scheduling or WiFi |
| 2026-10-08 | S1-U04 | Wireless TEST image | On-target run pending | PENDING | No Pico console log | Compiled self-check is not an executed on-target result |
