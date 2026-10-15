#ifndef MISSION_GUARDS_H
#define MISSION_GUARDS_H
#include "ir_snapshot.h"
#include "../common/robot_config.h"
#include "../subsystem2/subsystem2.h"
#include "../subsystem4/subsystem4.h"
typedef struct
{
    uint32_t first_ms, last_ms, sequence;
    unsigned count;
} MissionDwell;
bool mission_sensors_ready(uint32_t now, const S3IrSnapshot *ir,
                           const MotionFeedback *motion,
                           const TerrainSnapshot *terrain);
/* Mapping is configuration only. Raw encoder quietness and acknowledged
   braking qualify stopped replacement before speed/PID calibration exists. */
bool mission_mapping_stopped(uint32_t now, const MotionFeedback *motion,
                             const MotionHealth *health);
uint32_t mission_broad_observe(MissionDwell *dwell, const S3IrSnapshot *ir,
                               uint32_t now);
bool mission_junction_check(const RobotConfig *config, float distance,
                            uint32_t held_ms);
bool mission_finish_check(const RobotConfig *config, uint32_t actions,
                          float distance, uint32_t held_ms);
float mission_pivot_radius(const RobotConfig *config);
float mission_obstacle_trigger(const RobotConfig *config);
#endif
