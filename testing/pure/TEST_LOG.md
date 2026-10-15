# Pure algorithm host checks

These supplemental checks compile and execute the production's deterministic C modules directly with the host C compiler and real project headers. They use no T-Kernel stubs, fake kernel, hardware emulation, or replacement production code. They complement the target `TEST=1` fixtures and do not count as firmware or physical evidence.

| Check | Production behavior | Result |
| --- | --- | --- |
| S1-MAPPING | Mapping parser accepts max request ID across fragments and rejects leading-zero ID | Pending individual case run |
| S1-TELEMETRY | Fresh telemetry retains independent producer values through clock wrap | Pending individual case run |
| S2-MOTION | Distance command renewal preserves origin/deadline, waits for both wheels, and accepts completion ACK | Pending individual case run |
| S3-BARCODE-AZ | A-Z vectors decode at two scales and both orientations | Pending individual case run |
| S3-IR-CAPTURE | Black/white captures produce fresh calibrated steering; stale output is invalid | Pending individual case run |
| S4-TERRAIN-CAL | One hundred gravity-consistent samples complete calibration | Pending individual case run |
| S5-PROFILE | Valid scan rays produce profile geometry with the configured forward sensor offset | Pending individual case run |

Run `sh testing/pure/run.sh`. These checks exercise only callable pure functions; task scheduling, T-Kernel synchronization, I/O, and hardware acceptance remain outside this runner.
