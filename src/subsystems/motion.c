#include "robot/subsystems.h"

void motion_init(void) {
    /* TODO Buddy 2: configure driver disabled, PWM and encoder interrupts. */
    motion_stop();
}

void motion_update(MotionTarget target, uint32_t now_ms) {
    (void)target;
    (void)now_ms;
    /* TODO: encoder speed, per-wheel PID, saturation and command watchdog.
       Zero targets must stop outputs and reset PID integral state. */
}

MotionFeedback motion_read(void) {
    /* TODO: return measured encoder feedback, never copy requested targets. */
    return (MotionFeedback){0};
}

void motion_stop(void) {
    /* TODO: physically disable both motor channels, reset PID state. */
}
