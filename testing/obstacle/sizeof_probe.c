/* Cortex-M0+ storage evidence for obstacle bit packing (#111). */
#include "../../app_program/obstacle/obstacle.h"
#define PROBE(name, n) char probe_##name[(n)]
#define SIZE(type) PROBE(sizeof_##type, sizeof(type))
SIZE(ObstacleRay);
SIZE(ObstacleProfile);
/* Bit-fields are not addressable and cannot be operands of sizeof. */
