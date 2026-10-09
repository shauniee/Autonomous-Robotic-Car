#ifndef ROBOT_NAVIGATION_H
#define ROBOT_NAVIGATION_H
#include "subsystem3/barcode.h"
#include "subsystem3/ir_snapshot.h"
#include "subsystem2/navigation.h"

typedef struct
{
    bool configured;
    float follow_mps, action_mps, search_mps;
    float junction_min_m, junction_max_m, straight_m, turn_deg, uturn_deg;
    float reacquire_min_m, reacquire_max_m;
    uint32_t approach_ms, action_ms, reacquire_ms;
} RobotNavigationConfig;
typedef enum { ROBOT_NAV_IDLE, ROBOT_NAV_APPROACH,
    ROBOT_NAV_ACTION, ROBOT_NAV_REACQUIRE, ROBOT_NAV_FAILED } RobotNavPhase;
typedef struct
{
    RobotNavPhase phase;
    S3NavEvent event;
    uint32_t context, started_ms, last_sequence;
    unsigned visible_samples;
    float origin_left, origin_right;
    const char *reason;
    bool acknowledge, completed;
} RobotNavigation;

bool robot_navigation_config_valid(const RobotNavigationConfig *config,
    const S2NavigationConfig *motion);
void robot_navigation_reset(RobotNavigation *navigation, uint32_t context);
bool robot_navigation_accept(RobotNavigation *navigation,
    const S3NavEvent *event, uint32_t generation, uint32_t max_age_ms,
    uint32_t now);
bool robot_navigation_tick(RobotNavigation *navigation,
    const RobotNavigationConfig *config, const S2NavigationConfig *motion,
    const S3IrSnapshot *ir, const S2EncoderSnapshot *encoder,
    const S2NavigationFeedback *feedback, uint32_t now,
    S2NavigationRequest *request);
/* Startup-only; unavailable calibration leaves opt-in mode inhibited. */
bool robot_navigation_load(RobotNavigationConfig *config,
    S2NavigationConfig *motion, S3BarcodeConfig *barcode);
#endif
