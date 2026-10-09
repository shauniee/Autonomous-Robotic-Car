#ifndef ROBOT_CONTROL_POLICY_H
#define ROBOT_CONTROL_POLICY_H

#include "subsystem3/ir_snapshot.h"

#define ROBOT_BASE_DUTY 0.10f
#define ROBOT_MAX_DUTY 0.35f
#define ROBOT_SLOW_DUTY 0.05f
#define ROBOT_LOSS_MS 100u
#define ROBOT_DEBOUNCE_MS 30u
#define ROBOT_RESET_MS 60u

typedef enum
{
    ROBOT_READY, ROBOT_START_PENDING, ROBOT_RESET_PENDING,
    ROBOT_RUNNING, ROBOT_RECOVERING, ROBOT_STOPPED, ROBOT_FAULT
} RobotState;

typedef struct
{
    bool available;
    bool stopped;
    bool braked;
    bool driver_ok;
    uint32_t generation;
    uint32_t brake_ack;
    uint32_t reset_ack;
} RobotMotionState;

typedef struct
{
    RobotState state;
    const char *reason;
    float left_duty;
    float right_duty;
    int last_direction;
    unsigned reacquire_samples;
    uint32_t loss_ms;
    uint32_t last_ir_sequence;
    uint32_t reset_generation;
    uint32_t reset_ms;
    uint32_t capture_ms;
    bool have_ir_sequence;
    bool capture_pending;
    bool button_candidate;
    bool button_stable;
    bool button_armed;
    uint32_t button_changed_ms;
    bool stop_previous;
    bool request_stop;
    bool request_reset;
    bool cancel_capture;
    IrCaptureKind capture_request;
} RobotPolicy;

typedef struct
{
    uint32_t ir_sequence;
    uint32_t motion_sequence;
    bool have_progress;
} RobotWatchdogPolicy;

bool robot_watchdog_progress(RobotWatchdogPolicy *policy, bool valid,
    uint32_t ir_sequence, uint32_t sampled_ms, uint32_t motion_sequence,
    uint32_t serviced_ms, uint32_t now);
void robot_policy_init(RobotPolicy *policy);
void robot_policy_stop(RobotPolicy *policy, const char *reason, bool fault);
void robot_policy_tick(RobotPolicy *policy, const S3IrSnapshot *ir,
    const RobotMotionState *motion, bool start_down, bool stop_down,
    uint32_t now, bool control_due);
bool robot_policy_moving(const RobotPolicy *policy);
const char *robot_policy_state_name(RobotState state);

#endif
