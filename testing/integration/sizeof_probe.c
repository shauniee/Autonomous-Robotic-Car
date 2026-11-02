/* Compile-only target ABI evidence for the current three-task software. */
#include "../../app_program/mission/mission_logic.h"
#include "../../app_program/motion/motion.h"
#include "../../app_program/terrain/terrain.h"
#include "../../app_program/obstacle/obstacle.h"
#include "../../app_program/comms/comms.h"

#define PROBE(type) char probe_sizeof_##type[sizeof(type)]
PROBE(Mission);
PROBE(MotionCommand);
PROBE(MotionHealth);
PROBE(TerrainContext);
PROBE(TerrainSnapshot);
PROBE(obstacle_range_sample_t);
PROBE(obstacle_range_group_t);
PROBE(obstacle_profile_evidence_t);
PROBE(obstacle_profile_config_t);
PROBE(ObstacleRay);
PROBE(ObstacleProfile);
PROBE(obstacle_profile_snapshot_t);
PROBE(rd_word_t);
PROBE(mqtt_mapping_t);
PROBE(mqtt_ack_t);
PROBE(mqtt_telemetry_t);
PROBE(mqtt_event_t);
PROBE(comms_event_log_t);
PROBE(recovery_control_t);
PROBE(bypass_control_t);

_Static_assert(sizeof(obstacle_range_group_t) == 12u, "range group ABI");
_Static_assert(sizeof(obstacle_profile_evidence_t) == 6u, "profile evidence ABI");
_Static_assert(sizeof(obstacle_range_sample_t) == 16u, "range acquisition ABI");
