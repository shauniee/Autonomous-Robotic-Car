#include "robot/controller.h"
#include "robot/config.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static SensorFrame clear_frame(uint32_t now_ms) {
    return (SensorFrame){
        .line = {.status = {true, now_ms}, .detected = true},
        .terrain = {.status = {true, now_ms}},
        .obstacle = {.status = {true, now_ms}, .front_distance_m = 1.0f}
    };
}

static void assert_stopped(VehicleController c) {
    assert(c.state != STATE_FOLLOW_LINE);
    assert(c.target.left_mps == 0.0f && c.target.right_mps == 0.0f);
}

int main(void) {
    VehicleController c;
    SensorFrame s = clear_frame(1000);
    controller_init(&c);
    controller_step(&c, &s, COMMAND_NONE, 1000);
    assert(c.state == STATE_IDLE);
    assert_stopped(c);

    controller_step(&c, &s, COMMAND_START, 1000);
    assert(c.state == STATE_FOLLOW_LINE);
    assert(c.target.left_mps == c.target.right_mps && c.target.left_mps > 0);
    s.line.error = -1.0f;
    controller_step(&c, &s, COMMAND_NONE, 1000);
    assert(c.target.left_mps < c.target.right_mps);
    s.line.error = 1.0f;
    controller_step(&c, &s, COMMAND_NONE, 1000);
    assert(c.target.left_mps > c.target.right_mps);
    assert(c.target.left_mps <= MAX_SPEED_MPS);

    s.obstacle.front_distance_m = OBSTACLE_STOP_M;
    controller_step(&c, &s, COMMAND_NONE, 1000);
    assert_stopped(c);
    s = clear_frame(1000);
    controller_step(&c, &s, COMMAND_NONE, 1000);
    assert_stopped(c); /* Clearing an obstacle must not auto-restart. */
    controller_step(&c, &s, COMMAND_START, 1000);
    assert(c.state == STATE_FOLLOW_LINE);
    controller_step(&c, &s, COMMAND_STOP, 1000);
    assert_stopped(c);

    for (int sensor = 0; sensor < 3; ++sensor) {
        s = clear_frame(1000);
        SampleStatus *status = sensor == 0 ? &s.line.status :
                               sensor == 1 ? &s.terrain.status : &s.obstacle.status;
        status->valid = false;
        controller_step(&c, &s, COMMAND_START, 1000);
        assert(c.state == STATE_FAULT);
        assert_stopped(c);
        status->valid = true;
        status->sampled_ms = 1000 - SENSOR_MAX_AGE_MS - 1;
        controller_step(&c, &s, COMMAND_START, 1000);
        assert(c.state == STATE_FAULT);
        assert_stopped(c);
    }

    s = clear_frame(1000);
    s.line.detected = false;
    controller_step(&c, &s, COMMAND_START, 1000);
    assert_stopped(c);
    s = clear_frame(1000);
    s.terrain.collision = true;
    controller_step(&c, &s, COMMAND_START, 1000);
    assert(c.state == STATE_FAULT);
    assert_stopped(c);
    s = clear_frame(1000);
    s.line.error = NAN;
    controller_step(&c, &s, COMMAND_START, 1000);
    assert(c.state == STATE_FAULT);
    assert_stopped(c);
    s = clear_frame(1000);
    s.obstacle.front_distance_m = INFINITY;
    controller_step(&c, &s, COMMAND_START, 1000);
    assert_stopped(c);

    for (int nav = NAV_LEFT; nav <= NAV_STOP; ++nav) {
        s = clear_frame(1000);
        s.line.navigation = (Navigation)nav;
        controller_step(&c, &s, COMMAND_START, 1000);
        assert_stopped(c);
    }
    s = clear_frame(1000);
    s.line.junction = true;
    controller_step(&c, &s, COMMAND_START, 1000);
    assert_stopped(c);
    s = clear_frame(1000);
    controller_step(&c, &s, (RobotCommand)99, 1000);
    assert(c.state == STATE_FAULT);
    assert_stopped(c);

    /* Monotonic millisecond counter can wrap on a long-running board. */
    s = clear_frame(UINT32_MAX - 10u);
    controller_step(&c, &s, COMMAND_START, 10u);
    assert(c.state == STATE_FOLLOW_LINE);
    puts("Controller checks passed.");
    return 0;
}
