#ifndef MOTION_TRACE_H
#define MOTION_TRACE_H

#include <stdbool.h>
#include <stdint.h>
#include "../subsystem1/mqtt_frame.h"

/* Single producer: Motion/core 1. Single consumer: I/O/core 0.
   Full drops newest; neither side waits or touches motor authority. */
bool motion_trace_publish(const mqtt_wheel_trace_t *trace);
unsigned motion_trace_take(uint8_t *out, unsigned capacity);

#endif
