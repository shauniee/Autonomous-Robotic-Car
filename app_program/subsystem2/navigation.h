#ifndef S2_NAVIGATION_H
#define S2_NAVIGATION_H
#include "encoder.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum { S2_SPEED, S2_DISTANCE, S2_TURN } S2ActionKind;
typedef enum { S2_ACTION_IDLE, S2_ACTION_ACTIVE, S2_ACTION_DONE,
    S2_ACTION_FAILED } S2ActionState;
typedef struct
{
    bool configured;
    float diameter_m, track_m, counts_per_rev;
    int left_polarity, right_polarity;
    float kp, ki, kd, feedforward, integral_limit;
    float max_speed_mps, tolerance_m, stall_duty;
    uint32_t stall_ms;
} S2NavigationConfig;
typedef struct
{
    S2ActionKind kind;
    uint32_t id, acknowledge_id, timeout_ms;
    float left_mps, right_mps, target, speed_mps;
} S2NavigationRequest;
typedef struct
{
    bool encoder_valid, configured;
    uint32_t sampled_ms, action_id;
    S2ActionState action_state;
    float left_m, right_m, left_mps, right_mps, yaw_deg;
    const char *reason;
} S2NavigationFeedback;
typedef struct
{
    S2NavigationFeedback feedback;
    S2EncoderSnapshot previous;
    bool have_previous;
    uint32_t previous_ms, started_ms, last_move_left, last_move_right;
    uint32_t last_id;
    S2NavigationRequest action;
    float origin_left, origin_right, goal_left, goal_right;
    float integral[2], previous_error[2];
} S2NavigationController;

bool s2_navigation_config_valid(const S2NavigationConfig *config);
void s2_navigation_reset(S2NavigationController *controller);
bool s2_navigation_tick(S2NavigationController *controller,
    const S2NavigationConfig *config, const S2EncoderSnapshot *encoders,
    const S2NavigationRequest *request, uint32_t now,
    float *left_duty, float *right_duty);
#endif
