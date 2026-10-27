/* S1 minimum-bit sizeof probe (issue #112). Compile-only with:
   testing/subsystem3/sizeof_probe.sh testing/subsystem1/sizeof_probe.c
   Use the target compiler; a host sizeof check alone does not prove RP2040 RAM use. */
#include "../../app_program/subsystem1/s1_report.h"

#define PROBE(name, n) char probe_##name[(n)]
#define SIZE(type) PROBE(sizeof_##type, sizeof(type))
#define FIELD(type, field) PROBE(field_##type##__##field, sizeof(((type *)0)->field))

SIZE(mqtt_event_t);
SIZE(s1_packed_event_t);
FIELD(s1_packed_event_t, kind_arg);
FIELD(s1_packed_event_t, time_ms);
FIELD(s1_packed_event_t, left_count);
FIELD(s1_packed_event_t, right_count);
FIELD(s1_packed_event_t, arg2);

SIZE(s1_event_log_t);
FIELD(s1_event_log_t, events);
FIELD(s1_event_log_t, indices);
FIELD(s1_event_log_t, dropped);
