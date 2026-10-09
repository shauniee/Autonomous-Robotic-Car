#ifndef ROBOT_NAVIGATION_CONFIG_H
#define ROBOT_NAVIGATION_CONFIG_H
/* Copy to ignored config/navigation_config.h. Supply measured values and
   reviewed course decisions; zero is intentionally not usable calibration.
   Type definitions are included by robot_navigation_config.c. */
static const S2NavigationConfig robot_motion_configuration = {
    .configured = false,
    .diameter_m = 0, .track_m = 0, .counts_per_rev = 0,
    .left_polarity = 0, .right_polarity = 0,
    .kp = 0, .ki = 0, .kd = 0, .feedforward = 0, .integral_limit = 0,
    .max_speed_mps = 0, .tolerance_m = 0, .stall_duty = 0, .stall_ms = 0
};
static const S3BarcodeConfig robot_barcode_configuration = {
    .configured = false, .black_high = false, .allow_reverse = false,
    .left_polarity = 0, .right_polarity = 0,
    .narrow_min_counts = 0, .narrow_max_counts = 0,
    .wide_ratio_min = 0, .wide_ratio_max = 0, .narrow_variation = 0,
    .quiet_counts = 0, .rearm_counts = 0,
    .frame_timeout_ms = 0, .event_age_ms = 0,
    /* Assignment examples, not a confirmed course command table. Z unmapped. */
    .mapping = {[0] = S3_NAV_LEFT, [1] = S3_NAV_RIGHT,
        [2] = S3_NAV_STRAIGHT, [3] = S3_NAV_UTURN}
};
static const RobotNavigationConfig robot_route_configuration = {
    .configured = false, .follow_mps = 0, .action_mps = 0, .search_mps = 0,
    .junction_min_m = 0, .junction_max_m = 0,
    .straight_m = 0, .turn_deg = 0, .uturn_deg = 0,
    .reacquire_min_m = 0, .reacquire_max_m = 0,
    .approach_ms = 0, .action_ms = 0, .reacquire_ms = 0
};
#endif
