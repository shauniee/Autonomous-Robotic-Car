#include "navigation.h"
#include "motion_policy.h"
#include <math.h>
#include <stddef.h>

#define PI_F 3.14159265358979323846f
static bool positive(float value)
{
    return isfinite(value) && (value > 0.0f);
}
static float limit(float value, float bound)
{
    return fminf(bound, fmaxf(-bound, value));
}
bool s2_navigation_config_valid(const S2NavigationConfig *c)
{
    return (c != NULL) && c->configured && positive(c->diameter_m) &&
        positive(c->track_m) && positive(c->counts_per_rev) &&
        ((c->left_polarity == 1) || (c->left_polarity == -1)) &&
        ((c->right_polarity == 1) || (c->right_polarity == -1)) &&
        positive(c->kp) && isfinite(c->ki) && (c->ki >= 0.0f) &&
        isfinite(c->kd) && (c->kd >= 0.0f) &&
        isfinite(c->feedforward) && (c->feedforward >= 0.0f) &&
        positive(c->integral_limit) && positive(c->max_speed_mps) &&
        positive(c->tolerance_m) && positive(c->stall_duty) &&
        (c->stall_duty <= MOTION_MAX_DUTY) && (c->stall_ms > 60u) &&
        (c->stall_ms < INT32_MAX) &&
        positive(PI_F * c->diameter_m / c->counts_per_rev);
}
void s2_navigation_reset(S2NavigationController *c)
{
    *c = (S2NavigationController){.feedback.reason = "unconfigured"};
}
static bool fail(S2NavigationController *c, const char *reason)
{
    c->feedback.reason = reason;
    c->feedback.action_state = S2_ACTION_FAILED;
    return false;
}
static float pid(S2NavigationController *c, const S2NavigationConfig *config,
    unsigned wheel, float target, float measured, float dt)
{
    if (target == 0.0f)
    {
        c->integral[wheel] = 0.0f;
        c->previous_error[wheel] = 0.0f;
        return 0.0f;
    }
    float error = target - measured;
    float integral = limit(c->integral[wheel] + error * dt,
        config->integral_limit);
    float output = config->kp * error + config->ki * integral +
        config->kd * (error - c->previous_error[wheel]) / dt +
        config->feedforward * target;
    /* Conditional integration prevents accumulating into saturation. */
    if ((fabsf(output) <= MOTION_MAX_DUTY) || (output * error < 0.0f))
    {
        c->integral[wheel] = integral;
    }
    c->previous_error[wheel] = error;
    return limit(output, MOTION_MAX_DUTY);
}
static bool reached(float goal, float travel, float tolerance)
{
    return goal >= 0.0f ? travel >= goal - tolerance :
        travel <= goal + tolerance;
}
static bool same_action(const S2NavigationRequest *a,
    const S2NavigationRequest *b)
{
    return (a->id == b->id) && (a->kind == b->kind) &&
        (a->target == b->target) && (a->speed_mps == b->speed_mps) &&
        (a->timeout_ms == b->timeout_ms);
}
bool s2_navigation_tick(S2NavigationController *c,
    const S2NavigationConfig *config, const S2EncoderSnapshot *e,
    const S2NavigationRequest *r, uint32_t now, float *left, float *right)
{
    if ((c == NULL) || (left == NULL) || (right == NULL))
    {
        return false;
    }
    *left = 0.0f;
    *right = 0.0f;
    c->feedback.configured = s2_navigation_config_valid(config);
    c->feedback.encoder_valid = (e != NULL) && e->valid;
    if (!c->feedback.configured || !c->feedback.encoder_valid || (r == NULL))
    {
        return fail(c, "navigation_unconfigured");
    }
    if (!c->have_previous)
    {
        c->previous = *e;
        c->previous_ms = now;
        c->last_move_left = now;
        c->last_move_right = now;
        c->have_previous = true;
        c->feedback.reason = "encoder_baseline";
        return true;
    }
    uint32_t elapsed = now - c->previous_ms;
    if (elapsed == 0u)
    {
        return true;
    }
    if (elapsed > MOTION_EXPIRY_MS)
    {
        return fail(c, "encoder_service_gap");
    }
    float dt = (float)elapsed * 0.001f;
    float scale = PI_F * config->diameter_m / config->counts_per_rev;
    float dl = (float)(int32_t)(e->left_count - c->previous.left_count) *
        scale * (float)config->left_polarity;
    float dr = (float)(int32_t)(e->right_count - c->previous.right_count) *
        scale * (float)config->right_polarity;
    c->previous = *e;
    c->previous_ms = now;
    c->feedback.sampled_ms = now;
    c->feedback.left_m += dl;
    c->feedback.right_m += dr;
    c->feedback.left_mps = dl / dt;
    c->feedback.right_mps = dr / dt;
    c->feedback.yaw_deg = (c->feedback.right_m - c->feedback.left_m) *
        180.0f / (PI_F * config->track_m);
    if (dl != 0.0f)
    {
        c->last_move_left = now;
    }
    if (dr != 0.0f)
    {
        c->last_move_right = now;
    }
    if (!isfinite(c->feedback.yaw_deg) ||
        !isfinite(c->feedback.left_mps) || !isfinite(c->feedback.right_mps))
    {
        return fail(c, "encoder_numeric_fault");
    }
    if ((c->feedback.action_state == S2_ACTION_DONE) &&
        (r->acknowledge_id == c->feedback.action_id))
    {
        c->feedback.action_state = S2_ACTION_IDLE;
    }
    float target_left = r->left_mps;
    float target_right = r->right_mps;
    if (r->kind != S2_SPEED)
    {
        if ((r->kind != S2_DISTANCE) && (r->kind != S2_TURN))
        {
            return fail(c, "action_kind_invalid");
        }
        if ((r->id == 0u) || !isfinite(r->target) ||
            (r->target == 0.0f) || !positive(r->speed_mps) ||
            (r->speed_mps > config->max_speed_mps) ||
            (r->timeout_ms == 0u) || (r->timeout_ms >= INT32_MAX))
        {
            return fail(c, "action_parameters_invalid");
        }
        if (r->id != c->feedback.action_id)
        {
            if ((c->feedback.action_state != S2_ACTION_IDLE) ||
                (r->id <= c->last_id))
            {
                return fail(c, "action_replay_or_busy");
            }
            c->action = *r;
            c->last_id = r->id;
            c->feedback.action_id = r->id;
            c->feedback.action_state = S2_ACTION_ACTIVE;
            c->started_ms = now;
            c->last_move_left = now;
            c->last_move_right = now;
            c->origin_left = c->feedback.left_m;
            c->origin_right = c->feedback.right_m;
            float arc = r->target * PI_F * config->track_m / 360.0f;
            c->goal_left = r->kind == S2_DISTANCE ? r->target : -arc;
            c->goal_right = r->kind == S2_DISTANCE ? r->target : arc;
        }
        else if (!same_action(&c->action, r))
        {
            return fail(c, "action_mutated");
        }
        if (c->feedback.action_state == S2_ACTION_DONE)
        {
            c->feedback.reason = "action_done";
            return true;
        }
        if (c->feedback.action_state != S2_ACTION_ACTIVE)
        {
            return fail(c, "action_already_acknowledged");
        }
        if ((uint32_t)(now - c->started_ms) >= c->action.timeout_ms)
        {
            return fail(c, "action_deadline");
        }
        bool done_left = reached(c->goal_left,
            c->feedback.left_m - c->origin_left, config->tolerance_m);
        bool done_right = reached(c->goal_right,
            c->feedback.right_m - c->origin_right, config->tolerance_m);
        if (done_left && done_right)
        {
            c->feedback.action_state = S2_ACTION_DONE;
            c->feedback.reason = "action_done";
            return true;
        }
        target_left = done_left ? 0.0f :
            copysignf(r->speed_mps, c->goal_left);
        target_right = done_right ? 0.0f :
            copysignf(r->speed_mps, c->goal_right);
    }
    else if (c->feedback.action_state == S2_ACTION_ACTIVE)
    {
        return fail(c, "action_preempted_without_stop");
    }
    if (!isfinite(target_left) || !isfinite(target_right) ||
        (fabsf(target_left) > config->max_speed_mps) ||
        (fabsf(target_right) > config->max_speed_mps))
    {
        return fail(c, "speed_target_invalid");
    }
    *left = pid(c, config, 0u, target_left, c->feedback.left_mps, dt);
    *right = pid(c, config, 1u, target_right, c->feedback.right_mps, dt);
    if (!isfinite(*left) || !isfinite(*right))
    {
        *left = 0.0f;
        *right = 0.0f;
        return fail(c, "pid_numeric_fault");
    }
    bool stall = ((target_left != 0.0f) &&
        (fabsf(*left) >= config->stall_duty) &&
        ((uint32_t)(now - c->last_move_left) >= config->stall_ms)) ||
        ((target_right != 0.0f) && (fabsf(*right) >= config->stall_duty) &&
        ((uint32_t)(now - c->last_move_right) >= config->stall_ms));
    if (stall)
    {
        *left = 0.0f;
        *right = 0.0f;
        return fail(c, "encoder_stall");
    }
    c->feedback.reason = "wheel_speed_control";
    return true;
}
