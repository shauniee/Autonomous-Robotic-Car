/* Cortex-M0+ compile-only storage evidence for #110. */
#include "../../app_program/subsystem4/terrain_algo.h"

#define PROBE(name, n) char probe_##name[(n)]
#define SIZE(type) PROBE(sizeof_##type, sizeof(type))
#define FIELD(type, field) PROBE(field_##type##__##field, sizeof(((type *)0)->field))

SIZE(TerrainCalibSum);
SIZE(TerrainFilterQ16);
SIZE(TerrainContext);
FIELD(TerrainContext, filtered_accel_q16);
FIELD(TerrainContext, calib_sum);
FIELD(TerrainContext, rise_q);
FIELD(TerrainContext, pitch_cdeg);
FIELD(TerrainContext, hump_count);
