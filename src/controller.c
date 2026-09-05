#include "robot/controller.h"
#include "robot/config.h"
#include <math.h>

static bool fresh(SampleStatus status, uint32_t now_ms) {
    return status.valid && (uint32_t)(now_ms - status.sampled_ms) <= SENSOR_MAX_AGE_MS;
}

static void stop(VehicleController *c, MissionState state, const char *reason) {
    c->state = state;
    c->target = (MotionTarget){0};
    c->reason = reason;
}

static float limit_speed(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > MAX_SPEED_MPS) return MAX_SPEED_MPS;
    return value;
}

void controller_init(VehicleController *c) {
    stop(c, STATE_IDLE, "awaiting_start");
}

void controller_step(VehicleController *c, const SensorFrame *s,
                     RobotCommand command, uint32_t now_ms) {
    if (command == COMMAND_STOP) {
        stop(c, STATE_STOPPED, "command_stop");
        return;
    }
    if (command != COMMAND_NONE && command != COMMAND_START) {
        stop(c, STATE_FAULT, "invalid_command");
        return;
    }
    /* A stop/fault stays latched until a NEW start command arrives. */
    if (c->state != STATE_FOLLOW_LINE && command != COMMAND_START) return;

    if (!fresh(s->line.status, now_ms) || !fresh(s->terrain.status, now_ms) ||
        !fresh(s->obstacle.status, now_ms)) {
        stop(c, STATE_FAULT, "sensor_invalid_or_stale");
        return;
    }
    if (!isfinite(s->line.error) || s->line.error < -1.0f || s->line.error > 1.0f ||
        !isfinite(s->obstacle.front_distance_m) || s->obstacle.front_distance_m < 0.0f) {
        stop(c, STATE_FAULT, "sensor_out_of_range");
        return;
    }
    if (s->terrain.collision) {
        stop(c, STATE_FAULT, "collision");
        return;
    }
    if (s->obstacle.front_distance_m <= OBSTACLE_STOP_M) {
        stop(c, STATE_STOPPED, "obstacle_avoidance_todo");
        return;
    }
    if (!s->line.detected) {
        stop(c, STATE_STOPPED, "line_recovery_todo");
        return;
    }
    if (s->line.navigation == NAV_STOP) {
        stop(c, STATE_STOPPED, "barcode_stop");
        return;
    }
    if (s->line.junction || s->line.navigation != NAV_NONE) {
        stop(c, STATE_STOPPED, "navigation_execution_todo");
        return;
    }
    c->state = STATE_FOLLOW_LINE;
    c->reason = "following_line";
    const float correction = LINE_KP * s->line.error;
    c->target.left_mps = limit_speed(BASE_SPEED_MPS + correction);
    c->target.right_mps = limit_speed(BASE_SPEED_MPS - correction);
}

const char *controller_state_name(MissionState state) {
    switch (state) {
        case STATE_IDLE: return "IDLE";
        case STATE_FOLLOW_LINE: return "FOLLOW_LINE";
        case STATE_STOPPED: return "STOPPED";
        case STATE_FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}
