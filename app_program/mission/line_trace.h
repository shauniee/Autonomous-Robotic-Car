#ifndef LINE_TRACE_H
#define LINE_TRACE_H

#include <stdbool.h>
#include <stdint.h>
#include "../comms/mqtt_frame.h"

/* Single producer: Mission. Single consumer: I/O.
   Full drops newest; neither side waits or touches motor authority. */
bool line_trace_publish(const mqtt_line_trace_t *trace);
unsigned line_trace_take(uint8_t *out, unsigned capacity);

#endif
