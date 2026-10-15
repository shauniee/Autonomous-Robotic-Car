#include "robot_config.h"
#include "../subsystem3/barcode_decode.h"
#include <math.h>

/* Intentionally unconfigured. Replace zeros only with recorded measurements.
   finish_black_ms selects a CONFIRMED sustained-black finish marker after
   the configured action count and distance; unsupported finish rules inhibit.
 */
const RobotConfig robot_config = {
    /* Preliminary manual-turn geometry: testing/subsystem2/logs/
       2026-10-15_wheel-measurements.json. Ground accuracy/PID still pending;
       confirmation flags remain false until powered acceptance. */
    .left_m_per_count = 0.0000806969f,
    .right_m_per_count = 0.0000808247f,
    .track_m = 0.113f,
    .left_encoder_sign = -1,
                                  .right_encoder_sign = -1,
                                  .left_forward_a = true,
                                  .right_forward_a = true,
                                  .lateral_axis = 0u,
                                  .forward_axis = 1u,
                                  .vertical_axis = 2u,
                                  .lateral_sign = 1,
                                  .forward_sign = 1,
                                  .vertical_sign = 1,
                                  .sonar_repeat_ms = 60u,
                                  .servo_settle_ms = 300u,
                                  .scan_ms = 6000u,
                                  .action_ms = 5000u,
                                  .hump_ms = 6000u};

static bool positive(float value)
{
    return isfinite(value) && value > 0.0f;
}

static bool motion_valid(const RobotConfig *c)
{
    return c->motion_confirmed && positive(c->left_m_per_count) &&
           positive(c->right_m_per_count) && positive(c->track_m) &&
           c->left_m_per_count <= 0.1f && c->right_m_per_count <= 0.1f &&
           c->track_m <= 2.0f &&
           (c->left_encoder_sign == 1 || c->left_encoder_sign == -1) &&
           (c->right_encoder_sign == 1 || c->right_encoder_sign == -1) &&
           positive(c->speed_max_mps) && positive(c->cruise_mps) &&
           positive(c->search_mps) && c->cruise_mps <= c->speed_max_mps &&
           c->search_mps <= c->cruise_mps && positive(c->kp) &&
           isfinite(c->ki) && c->ki >= 0.0f && isfinite(c->kd) &&
           c->kd >= 0.0f && positive(c->distance_tolerance_m) &&
           positive(c->angle_tolerance_deg) && positive(c->stationary_mps) &&
           c->stationary_mps < c->search_mps && c->stall_ms > 60u &&
           c->stall_ms < 10000u && c->action_ms > 60u && c->action_ms < 60000u;
}

static bool sonar_valid(const RobotConfig *c)
{
    return c->sonar_confirmed && c->servo_min_us >= 500u &&
           c->servo_min_us < c->servo_center_us &&
           c->servo_center_us < c->servo_max_us && c->servo_max_us <= 2500u &&
           positive(c->servo_us_per_deg) &&
           (float)c->servo_center_us - 60.0f * c->servo_us_per_deg >=
               c->servo_min_us &&
           (float)c->servo_center_us + 60.0f * c->servo_us_per_deg <=
               c->servo_max_us &&
           (c->servo_left_sign == 1 || c->servo_left_sign == -1) &&
           c->servo_settle_ms >= 20u && c->servo_settle_ms <= 1000u &&
           c->sonar_repeat_ms >= 60u && c->sonar_repeat_ms <= 200u &&
           c->scan_ms >= 1000u && c->scan_ms < 30000u &&
           positive(c->sonar_min_m) && c->sonar_max_m > c->sonar_min_m &&
           c->sonar_max_m <= 5.0f && positive(c->body_width_m) &&
           positive(c->body_length_m) && positive(c->clearance_m) &&
           isfinite(c->sonar_forward_m) && isfinite(c->sonar_left_m) &&
           fabsf(c->sonar_forward_m) <= c->body_length_m * 0.5f &&
           fabsf(c->sonar_left_m) <= c->body_width_m * 0.5f;
}

static bool terrain_valid(const RobotConfig *c)
{
    return c->terrain_confirmed && c->lateral_axis < 3u &&
           c->forward_axis < 3u && c->vertical_axis < 3u &&
           c->lateral_axis != c->forward_axis &&
           c->forward_axis != c->vertical_axis &&
           c->lateral_axis != c->vertical_axis &&
           (c->lateral_sign == 1 || c->lateral_sign == -1) &&
           (c->forward_sign == 1 || c->forward_sign == -1) &&
           (c->vertical_sign == 1 || c->vertical_sign == -1) &&
           positive(c->height_factor) && isfinite(c->roll_height_factor) &&
           c->roll_height_factor >= 0.0f && positive(c->gravity_tolerance_g) &&
           c->gravity_tolerance_g < 0.5f && positive(c->impact_g) &&
           c->impact_g <= 2.0f && positive(c->hump_pitch_deg) &&
           c->hump_pitch_deg < 45.0f && positive(c->hump_roll_deg) &&
           c->hump_roll_deg < 45.0f &&
           positive(c->acceleration_threshold_mps2) &&
           positive(c->turn_threshold_deg_s) && c->hump_ms >= 500u &&
           c->hump_ms < 60000u;
}

static bool mission_valid(const RobotConfig *c)
{
    return c->commissioning == COMMISSIONING_DISABLED &&
           c->electrical_confirmed && c->course_confirmed && motion_valid(c) &&
           sonar_valid(c) && terrain_valid(c) &&
           barcode_config_valid(&c->barcode) &&
           c->barcode.left_um_per_count ==
               (uint32_t)(c->left_m_per_count * 1000000.0f + 0.5f) &&
           c->barcode.right_um_per_count ==
               (uint32_t)(c->right_m_per_count * 1000000.0f + 0.5f) &&
           positive(c->junction_min_m) &&
           c->junction_max_m > c->junction_min_m &&
           positive(c->junction_exit_m) && c->junction_hold_ms >= 20u &&
           c->junction_hold_ms <= 500u && positive(c->obstacle_depth_bound_m) &&
           positive(c->braking_headroom_m) &&
           hypotf(c->body_width_m, c->body_length_m) * 0.5f + c->clearance_m +
                   c->braking_headroom_m - c->sonar_forward_m <
               c->sonar_max_m &&
           positive(c->bypass_max_m) && positive(c->reacquire_max_m) &&
           positive(c->reacquire_heading_deg) &&
           positive(c->reacquire_corridor_m) &&
           c->reacquire_corridor_m <= c->reacquire_max_m &&
           c->reacquire_heading_deg <= 45.0f &&
           c->finish_rule == FINISH_BROAD_BLACK && positive(c->finish_min_m) &&
           c->finish_after_actions > 0u &&
           c->finish_black_ms > c->junction_hold_ms &&
           c->finish_black_ms <= 2000u;
}

bool robot_config_validate(const RobotConfig *c)
{
    return c != NULL && mission_valid(c);
}

bool robot_config_motion_valid(void)
{
    return motion_valid(&robot_config);
}

bool robot_config_sonar_valid(void)
{
    return sonar_valid(&robot_config);
}

bool robot_config_terrain_valid(void)
{
    return terrain_valid(&robot_config);
}

bool robot_config_mission_valid(void)
{
    return mission_valid(&robot_config);
}

bool robot_config_terrain_check(const RobotConfig *c)
{
    return c != NULL && terrain_valid(c);
}
