#ifndef MISSION_GUARDS_H
#define MISSION_GUARDS_H
#include "ir_snapshot.h"
#include "../common/robot_config.h"
typedef struct
{
    uint32_t first_ms, last_ms, sequence;
    unsigned count;
} MissionDwell;
uint32_t mission_broad_observe(MissionDwell *dwell, const S3IrSnapshot *ir,
                               uint32_t now);
bool mission_junction_check(const RobotConfig *config, float distance,
                            uint32_t held_ms);
bool mission_finish_check(const RobotConfig *config, uint32_t actions,
                          float distance, uint32_t held_ms);
float mission_pivot_radius(const RobotConfig *config);
float mission_obstacle_trigger(const RobotConfig *config);
#endif
