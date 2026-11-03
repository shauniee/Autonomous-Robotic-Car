/* Comms minimum-bit sizeof probe (issue #112). Compile-only with:
   testing/line_barcode/sizeof_probe.sh testing/comms/sizeof_probe.c
   Use the target compiler; a host sizeof check alone does not prove RP2040 RAM use. */
#include "../../app_program/comms/comms.h"

#define PROBE(name, n) char probe_##name[(n)]
#define SIZE(type) PROBE(sizeof_##type, sizeof(type))
#define FIELD(type, field) PROBE(field_##type##__##field, sizeof(((type *)0)->field))

SIZE(mqtt_event_t);
SIZE(comms_packed_event_t);
FIELD(comms_packed_event_t, kind_arg);
FIELD(comms_packed_event_t, time_ms);
FIELD(comms_packed_event_t, left_count);
FIELD(comms_packed_event_t, right_count);
FIELD(comms_packed_event_t, arg2);

SIZE(comms_event_log_t);
FIELD(comms_event_log_t, events);
FIELD(comms_event_log_t, indices);
FIELD(comms_event_log_t, dropped);
