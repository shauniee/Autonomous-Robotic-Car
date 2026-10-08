/* Nicholas: local controls, explicit recovery and requested motor duties. */
#ifndef ROBOT_CONTROL_H
#define ROBOT_CONTROL_H
#include "ir_tracking.h"

#define ROBOT_BASE_DUTY .10f
#define ROBOT_MAX_DUTY .35f
#define ROBOT_SLOW_DUTY .05f
#define ROBOT_LOSS_MS 100u
#define ROBOT_REACQUIRE_SAMPLES 2u

typedef enum { ROBOT_READY, ROBOT_RUNNING, ROBOT_RECOVERING, ROBOT_STOPPED } RobotState;
typedef struct {
    RobotState state;
    const char *reason;
    IrTracker tracking;
    bool stop_active;
    uint32_t loss_ms;
    unsigned reacquire_samples;
    float last_direction, left_duty, right_duty;
    bool button_candidate, button_stable, button_armed;
    uint32_t button_changed_ms;
    unsigned chirps; /* consumed by the asynchronous buzzer driver */
} RobotControl;

void robot_control_init(RobotControl *control);
bool robot_control_capture(RobotControl *control, IrCaptureKind kind, uint32_t now);
void robot_control_sample(RobotControl *control, IrRaw raw, bool acquired, uint32_t now);
void robot_control_check_time(RobotControl *control, uint32_t now);
void robot_control_stop(RobotControl *control, const char *reason);
bool robot_control_is_moving(const RobotControl *control);
/* Active-low inputs are already converted to logical down/stop. */
void robot_control_inputs(RobotControl *control, bool start_down, bool stop, uint32_t now);
const char *robot_control_state_name(RobotState state);
#endif
