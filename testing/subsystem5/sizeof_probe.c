/* Cortex-M0+ storage evidence for obstacle bit packing (#111). */
#include "../../app_program/subsystem5/obstacle_profile.h"
#define PROBE(name, n) char probe_##name[(n)]
#define SIZE(type) PROBE(sizeof_##type, sizeof(type))
#define FIELD(type, field) PROBE(field_##type##__##field, sizeof(((type *)0)->field))
SIZE(ObstacleRay);
FIELD(ObstacleRay, range_mm);
SIZE(ObstacleProfile);
FIELD(ObstacleProfile, closest_mm);
FIELD(ObstacleProfile, width_mm);
FIELD(ObstacleProfile, left_clearance_mm);
FIELD(ObstacleProfile, right_clearance_mm);
FIELD(ObstacleProfile, obstacle_forward_mm);
FIELD(ObstacleProfile, obstacle_left_mm);
