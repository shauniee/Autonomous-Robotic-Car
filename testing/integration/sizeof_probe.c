/* Compile-only target ABI evidence for the current three-task software. */
#include "../../app_program/mission/mission_logic.h"
#include "../../app_program/subsystem2/motion_health.h"
#include "../../app_program/subsystem4/terrain_algo.h"
#include "../../app_program/subsystem4/terrain_evidence.h"
#include "../../app_program/subsystem5/range_filter.h"
#include "../../app_program/subsystem5/profile_edges.h"

#define PROBE(type) char probe_sizeof_##type[sizeof(type)]
PROBE(Mission);
PROBE(MotionCommand);
PROBE(MotionHealth);
PROBE(TerrainContext);
PROBE(S4Snapshot);
PROBE(s5_range_sample_t);
PROBE(s5_range_group_t);
PROBE(s5_profile_evidence_t);
PROBE(s5_profile_config_t);
PROBE(ObstacleRay);
PROBE(ObstacleProfile);

_Static_assert(sizeof(s5_range_group_t) == 12u, "range group ABI");
_Static_assert(sizeof(s5_profile_evidence_t) == 6u, "profile evidence ABI");
_Static_assert(sizeof(s5_range_sample_t) == 12u, "range acquisition ABI");
