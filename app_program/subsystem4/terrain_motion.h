#ifndef TERRAIN_MOTION_H
#define TERRAIN_MOTION_H
#include "subsystem4.h"
#include "../subsystem2/subsystem2.h"
bool terrain_motion_measure(const MotionFeedback *current,
                            const MotionFeedback *previous, bool have_previous,
                            uint32_t now, float *turn, float *acceleration);
TerrainMovement terrain_movement_classify(const TerrainContext *context,
                                          const MotionFeedback *motion,
                                          bool valid, float turn,
                                          float acceleration,
                                          const RobotConfig *config);
#endif
