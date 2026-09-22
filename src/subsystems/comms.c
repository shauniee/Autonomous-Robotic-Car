#include "robot/subsystems.h"
#include "robot/comms_protocol.h"
#include <stdio.h>

/* Desktop / non-wireless build backend. The Pico W target selects
   comms_picow.c instead, with the same public interface. */

void comms_init(void) {
}

RobotCommand comms_poll(uint32_t now_ms) {
    (void)now_ms;
    return COMMAND_NONE;
}

void comms_publish(const VehicleController *c, const SensorFrame *s,
                   const MotionFeedback *motion, uint32_t now_ms) {
    char payload[COMMS_TELEMETRY_MAX];
    if (comms_encode_telemetry(payload, sizeof(payload), c, s, motion, now_ms)) {
        puts(payload);
    }
}
