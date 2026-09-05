#include "robot/subsystems.h"
#include "robot/controller.h"
#include <stdio.h>
#include <inttypes.h>

void comms_init(void) {
    /* TODO Buddy 1: connection state, heartbeat, reconnect, MQTT transport. */
}

RobotCommand comms_poll(uint32_t now_ms) {
    (void)now_ms;
    /* TODO: validate and consume each remote command once. */
    return COMMAND_NONE;
}

void comms_publish(const VehicleController *c, const SensorFrame *s,
                   const MotionFeedback *motion, uint32_t now_ms) {
    (void)s;
    (void)motion;
    /* Debug status only. Extend with measured values and validity for MQTT. */
    printf("{\"uptime_ms\":%" PRIu32 ",\"state\":\"%s\",\"reason\":\"%s\","
           "\"target_left_mps\":%.3f,\"target_right_mps\":%.3f}\n",
           now_ms, controller_state_name(c->state), c->reason,
           (double)c->target.left_mps, (double)c->target.right_mps);
}
