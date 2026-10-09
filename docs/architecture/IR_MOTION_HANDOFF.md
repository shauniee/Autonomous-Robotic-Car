# IR and Motion handoff

## Historical record

The earlier template bench reported unresolved motor operation and had no acknowledged-brake reset barrier. Its source/profile-specific results remain in [integration records](../../testing/integration/TEST_LOG.md); they do not validate the later implementation. On 2026-10-09 Nicholas reported that IR calibration, both motors and encoders were functioning. This supplied report is distinct from measured acceptance of the new integrated firmware.

## Current interface contract

[Motion](MOTION.md#current-duty-service-contract) owns duty, freshness, generation, stop/reset and output acknowledgement. [Line Tracking](LINE_TRACKING.md#modules-and-public-interface) owns the S3 sensor/capture interface. [Architecture](../ARCHITECTURE.md#implemented-line-tracking-integration) owns local coordination and recovery. These supersede the old S3-fed motor/telemetry arrangement.

## Reproducible software builds

Use the four current [build profiles](../GETTING_STARTED.md#build-profiles) sequentially. Firmware self-tests run with motors inhibited after the local gate. The target is Robo Pico with Pico W/WH; an ordinary Pico or different carrier requires reviewed wiring/profile changes.

## Verification and known limits

Use [operating steps](../GETTING_STARTED.md#power-and-flashing) and [test evidence](../../testing/README.md). PWM readback/brake acknowledgement is output evidence, not wheel stationarity. Slow MQTT cannot establish short recovery/brake timing. Physical following, stop latency and network-load margins require dated integrated runs with revision/profile/image checksum. Preserve one power source at a time.
