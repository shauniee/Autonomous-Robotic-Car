#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

#include <stdbool.h>
#include <stdint.h>
#include "../subsystem3/barcode_types.h"

/* These are measured/course inputs, not firmware-calibrated defaults.
   Configure while powered off; record provenance in PROJECT_SPECS D01-D10. */
typedef enum
{
    FINISH_UNCONFIGURED,
    FINISH_BROAD_BLACK
} FinishRule;
typedef enum
{
    COMMISSIONING_DISABLED,
    COMMISSIONING_DUTY,
    COMMISSIONING_SPEED
} CommissioningMode;
typedef struct
{
    bool motion_confirmed, sonar_confirmed, terrain_confirmed;
    bool course_confirmed;
    bool electrical_confirmed;
    CommissioningMode commissioning;
    float commissioning_duty;
    float left_m_per_count, right_m_per_count, track_m;
    int left_encoder_sign, right_encoder_sign;
    bool left_forward_a, right_forward_a;
    float speed_max_mps, cruise_mps, search_mps;
    float kp, ki, kd, distance_tolerance_m, angle_tolerance_deg;
    float stationary_mps;
    uint32_t stall_ms, action_ms;
    float body_width_m, body_length_m, clearance_m;
    float sonar_forward_m, sonar_left_m, braking_headroom_m;
    uint16_t servo_center_us, servo_min_us, servo_max_us;
    float servo_us_per_deg;
    int servo_left_sign;
    uint32_t servo_settle_ms, sonar_repeat_ms, scan_ms;
    float sonar_min_m, sonar_max_m;
    uint8_t lateral_axis, forward_axis, vertical_axis;
    int lateral_sign, forward_sign, vertical_sign;
    float height_factor, roll_height_factor, gravity_tolerance_g;
    float impact_g, hump_pitch_deg, hump_roll_deg;
    float acceleration_threshold_mps2, turn_threshold_deg_s;
    uint32_t hump_ms;
    float junction_min_m, junction_max_m, junction_exit_m;
    uint32_t junction_hold_ms;
    float obstacle_depth_bound_m;
    float bypass_max_m, reacquire_max_m, reacquire_heading_deg,
        reacquire_corridor_m;
    FinishRule finish_rule;
    float finish_min_m;
    uint32_t finish_after_actions, finish_black_ms;
    barcode_config_t barcode;
} RobotConfig;

extern const RobotConfig robot_config;
bool robot_config_motion_valid(void);
bool robot_config_sonar_valid(void);
bool robot_config_terrain_valid(void);
bool robot_config_mission_valid(void);
bool robot_config_validate(const RobotConfig *config);
bool robot_config_terrain_check(const RobotConfig *config);

#endif
