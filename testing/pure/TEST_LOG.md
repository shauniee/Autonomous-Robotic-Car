# Pure algorithm host checks

These initial supplemental smoke checks compile and execute selected production deterministic C modules directly with the host C compiler and real project headers. They are distinct from the exact target fixture runner in `run_fixtures.sh`. They use no T-Kernel stubs, fake kernel, hardware emulation, or replacement production code. They complement the target `TEST=1` fixtures and do not count as firmware or physical evidence.

| Check | Production behavior | Result |
| --- | --- | --- |
| S1-MAPPING | Mapping parser accepts max request ID across fragments and rejects leading-zero ID | PASS on harness-identical `8f74f2d`: `sh testing/pure/run.sh` (2026-10-15) |
| S1-TELEMETRY | Fresh telemetry retains independent producer values through clock wrap | PASS: `sh testing/pure/run.sh`, host `cc`, 2026-10-15 |
| S2-MOTION | Distance command renewal preserves origin/deadline, waits for both wheels, and accepts completion ACK | PASS: `sh testing/pure/run.sh`, host `cc`, 2026-10-15 |
| S3-BARCODE-AZ | A-Z vectors decode at two scales and both orientations | PASS: `sh testing/pure/run.sh`, host `cc`, 2026-10-15 |
| S3-IR-CAPTURE | Black/white captures produce fresh calibrated steering; stale output is invalid | PASS: `sh testing/pure/run.sh`, host `cc`, 2026-10-15 |
| S4-TERRAIN-CAL | One hundred gravity-consistent samples complete calibration | PASS: `sh testing/pure/run.sh`, host `cc`, 2026-10-15 |
| S5-PROFILE | Valid scan rays produce profile geometry with the configured forward sensor offset | PASS: `sh testing/pure/run.sh`, host `cc`, 2026-10-15 |

Run `sh testing/pure/run.sh`. These checks exercise only callable pure functions; task scheduling, T-Kernel synchronization, I/O, and hardware acceptance remain outside this runner.
