#include "robot_navigation.h"
#include <math.h>
#include "float_math.h"
#include <stddef.h>

#define PI_F 3.14159265358979323846f
static bool positive(float value)
{
    return isfinite(value) && (value > 0.0f);
}
static bool duration(uint32_t value)
{
    return (value > 0u) && (value < INT32_MAX);
}
bool robot_navigation_config_valid(const RobotNavigationConfig *c,
    const S2NavigationConfig *motion)
{
    return (c != NULL) && c->configured &&
        s2_navigation_config_valid(motion) && positive(c->follow_mps) &&
        (2.0f * c->follow_mps <= motion->max_speed_mps) &&
        positive(c->action_mps) && (c->action_mps <= motion->max_speed_mps) &&
        positive(c->search_mps) && (c->search_mps <= motion->max_speed_mps) &&
        isfinite(c->junction_min_m) && (c->junction_min_m >= 0.0f) &&
        positive(c->junction_max_m) &&
        (c->junction_max_m > c->junction_min_m) && positive(c->straight_m) &&
        (c->straight_m > motion->tolerance_m) && positive(c->turn_deg) &&
        positive(c->uturn_deg) && (c->turn_deg <= 180.0f) &&
        (c->uturn_deg <= 360.0f) &&
        isfinite(c->reacquire_min_m) && (c->reacquire_min_m >= 0.0f) &&
        positive(c->reacquire_max_m) &&
        (c->reacquire_max_m > c->reacquire_min_m) &&
        duration(c->approach_ms) && duration(c->action_ms) &&
        duration(c->reacquire_ms);
}
void robot_navigation_reset(RobotNavigation *n, uint32_t context)
{
    *n = (RobotNavigation){.context = context, .reason = "following"};
}
bool robot_navigation_accept(RobotNavigation *n, const S3NavEvent *event,
    uint32_t generation, uint32_t max_age_ms, uint32_t now)
{
    if ((n == NULL) || (event == NULL) || (n->phase != ROBOT_NAV_IDLE) ||
        (n->next_action_id == UINT32_MAX) ||
        (event->context == 0u) || (event->context != n->context) ||
        (event->generation != generation) || (event->id == 0u) ||
        ((unsigned)event->action < S3_NAV_LEFT) ||
        ((unsigned)event->action > S3_NAV_UTURN) ||
        ((uint32_t)(now - event->acquired_ms) > max_age_ms))
    {
        return false;
    }
    n->event = *event;
    n->action_id = ++n->next_action_id;
    n->phase = ROBOT_NAV_APPROACH;
    n->started_ms = now;
    n->reason = "approach_junction";
    return true;
}
static bool fail(RobotNavigation *n, const char *reason)
{
    n->phase = ROBOT_NAV_FAILED;
    n->reason = reason;
    return false;
}
bool robot_navigation_tick(RobotNavigation *n,
    const RobotNavigationConfig *c, const S2NavigationConfig *motion,
    const S3IrSnapshot *ir, const S2EncoderSnapshot *encoder,
    const S2NavigationFeedback *feedback, uint32_t now,
    S2NavigationRequest *request)
{
    if ((n == NULL) || (request == NULL))
    {
        return false;
    }
    *request = (S2NavigationRequest){.kind = S2_SPEED};
    n->completed = false;
    n->acknowledge = false;
    if (!robot_navigation_config_valid(c, motion) || (ir == NULL) ||
        !ir->have_sample || !ir->acquisition_valid || !ir->sample_fresh ||
        !ir->calibration_valid || !ir->normalized_valid ||
        ((uint32_t)(now - ir->sampled_ms) > IR_MAX_GAP_MS) ||
        (encoder == NULL) || !encoder->valid || (feedback == NULL) ||
        !feedback->encoder_valid ||
        ((uint32_t)(now - feedback->sampled_ms) > IR_MAX_GAP_MS))
    {
        return fail(n, "navigation_data_unavailable");
    }
    if (n->phase == ROBOT_NAV_FAILED)
    {
        return false;
    }
    if (n->phase == ROBOT_NAV_APPROACH)
    {
        float scale = PI_F * motion->diameter_m / motion->counts_per_rev;
        float left = (float)(int32_t)(encoder->left_count -
            n->event.left_count) * scale * (float)motion->left_polarity;
        float right = (float)(int32_t)(encoder->right_count -
            n->event.right_count) * scale * (float)motion->right_polarity;
        float travel = (left + right) * 0.5f;
        if (!isfinite(travel) || (left < 0.0f) || (right < 0.0f) ||
            (travel > c->junction_max_m) ||
            ((uint32_t)(now - n->started_ms) >= c->approach_ms))
        {
            return fail(n, "junction_window_expired");
        }
        if ((travel >= c->junction_min_m) &&
            (ir->line == IR_LINE_BROAD_BLACK))
        {
            n->phase = ROBOT_NAV_ACTION;
            n->started_ms = now;
            n->reason = "barcode_navigation";
        }
        else if (ir->line != IR_LINE_VISIBLE)
        {
            return fail(n, "junction_not_confirmed");
        }
    }
    if (n->phase == ROBOT_NAV_ACTION)
    {
        if ((uint32_t)(now - n->started_ms) >= c->action_ms)
        {
            return fail(n, "navigation_deadline");
        }
        if ((feedback->action_id == n->action_id) &&
            (feedback->action_state == S2_ACTION_FAILED))
        {
            return fail(n, "navigation_action_failed");
        }
        if ((feedback->action_id == n->action_id) &&
            (feedback->action_state == S2_ACTION_DONE))
        {
            request->acknowledge_id = n->action_id;
            n->acknowledge = true;
            n->phase = ROBOT_NAV_REACQUIRE;
            n->started_ms = now;
            n->origin_left = feedback->left_m;
            n->origin_right = feedback->right_m;
            n->visible_samples = 0u;
            n->last_sequence = ir->sequence;
            n->reason = "reacquire_expected_route";
        }
        else
        {
            *request = (S2NavigationRequest){.id = n->action_id,
                .kind = n->event.action == S3_NAV_STRAIGHT ?
                    S2_DISTANCE : S2_TURN,
                .target = n->event.action == S3_NAV_STRAIGHT ? c->straight_m :
                    (n->event.action == S3_NAV_LEFT ? c->turn_deg :
                     (n->event.action == S3_NAV_RIGHT ? -c->turn_deg :
                      c->uturn_deg)),
                .speed_mps = c->action_mps, .timeout_ms = c->action_ms};
            return true;
        }
    }
    if (n->phase == ROBOT_NAV_REACQUIRE)
    {
        float travel = robot_float_max(robot_float_abs(feedback->left_m - n->origin_left),
            robot_float_abs(feedback->right_m - n->origin_right));
        if (!isfinite(travel) || (travel > c->reacquire_max_m) ||
            ((uint32_t)(now - n->started_ms) >= c->reacquire_ms))
        {
            return fail(n, "route_reacquire_expired");
        }
        bool visible = (ir->line == IR_LINE_VISIBLE) && ir->guidance_valid &&
            (travel >= c->reacquire_min_m);
        if (!visible)
        {
            n->visible_samples = 0u;
        }
        else if (ir->sequence != n->last_sequence)
        {
            ++n->visible_samples;
        }
        n->last_sequence = ir->sequence;
        if (n->visible_samples >= 2u)
        {
            n->phase = ROBOT_NAV_IDLE;
            n->reason = "expected_route_reacquired";
            n->completed = true;
        }
        else
        {
            request->left_mps = n->event.action == S3_NAV_LEFT ?
                0.0f : c->search_mps;
            request->right_mps = n->event.action == S3_NAV_RIGHT ?
                0.0f : c->search_mps;
            return true;
        }
    }
    if (!ir->guidance_valid || !isfinite(ir->steering))
    {
        return fail(n, "navigation_guidance_invalid");
    }
    request->left_mps = c->follow_mps * (1.0f + ir->steering);
    request->right_mps = c->follow_mps * (1.0f - ir->steering);
    return true;
}
