# Communications: deferred robot bridge

Owner Shaun / S1. This migration uses WIFI=cyw43 NET=none MQTT=0. The kernel owns radio calls on core0, priority4; S1 is an inactive placeholder. No robot MQTT/dashboard commands or status are implemented. Historical dashboard, schema5 and network source are recoverable from `../../../recovery/pre-mtkernel-20261008/code-source.tar.gz`; [former guide](../history/pre-mtkernel/architecture/COMMUNICATIONS.md) is historical evidence, not current instructions.

The full mission still requires telemetry, status-only subscriptions and healthy local autonomy through network loss. Future S1 bridge priority20 exchanges bounded kernel objects with networking code running inside the service task. The template requires team/instructor agreement before shared networking library changes; this migration does not make those changes. No student task may call CYW43, lwIP or MQTT directly.

Use the motor-inhibited TEST image for USB console checks, with USB as the sole source. Disconnect USB before power-bank trials; battery calibration uses GP20/21 and GP22 cues. No simultaneous USB logging during those trials.

Future T12/T13 telemetry/reconnect validation and MQTT deployment remain pending.
