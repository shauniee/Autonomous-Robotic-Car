#ifndef MOTION_CONTROL_H
#define MOTION_CONTROL_H
#include "../common/robot_config.h"

typedef enum
{
    MOTION_BRAKE,
    MOTION_SPEED,
    MOTION_DISTANCE,
    MOTION_TURN,
    MOTION_DUTY_BENCH
} MotionMode;
typedef enum
{
    MOTION_IDLE,
    MOTION_ACTIVE,
    MOTION_DONE,
    MOTION_FAILED
} MotionStatus;
typedef enum
{
    MOTION_OK,
    MOTION_UNCONFIGURED,
    MOTION_EXPIRED,
    MOTION_BAD_COMMAND,
    MOTION_STALLED,
    MOTION_ENCODER_FAULT,
    MOTION_CANCELLED
} MotionReason;
typedef struct
{
    uint32_t action_id, run_epoch, sampled_ms, issued_ms;
    uint32_t expires_ms, deadline_ms;
    MotionMode mode;
    float left_mps, right_mps, target, speed_mps;
    float left_duty, right_duty;
} MotionCommand;
typedef struct
{
    float integral, previous_speed;
} WheelPid;
typedef struct
{
    MotionCommand command;
    MotionStatus status;
    MotionReason reason;
    uint32_t last_id, epoch, stalled_left_ms, stalled_right_ms;
    float origin_left, origin_right;
    WheelPid left_pid, right_pid;
} MotionController;
void motion_control_init(MotionController *control, uint32_t epoch);
bool motion_command_valid(const MotionCommand *command, uint32_t now,
                          uint32_t epoch, const RobotConfig *config);
bool motion_control_accept(MotionController *control,
                           const MotionCommand *command, float left_m,
                           float right_m, uint32_t now,
                           const RobotConfig *config);
void motion_control_step(MotionController *control, float left_m, float right_m,
                         float left_mps, float right_mps, float dt,
                         uint32_t now, const RobotConfig *config, float *left,
                         float *right);
bool motion_control_ack(MotionController *control, uint32_t action_id);
#endif
