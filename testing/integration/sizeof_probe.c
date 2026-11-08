/* Compile-only target ABI evidence for the current three-task software. */
#include "../../app_program/mission/mission_logic.h"
#include "../../app_program/motion/motion.h"
#include "../../app_program/terrain/terrain.h"
#include "../../app_program/obstacle/obstacle.h"
#include "../../app_program/comms/comms.h"
#include "../../app_program/mission/safety_logic.h"

#define PROBE(type) char probe_sizeof_##type[sizeof(type)]
PROBE(Button);
PROBE(ButtonInputs);
PROBE(SafetyInputs);
PROBE(Mission);
PROBE(MissionInputs);
PROBE(MissionOutputs);
PROBE(MotionCommand);
PROBE(MotionHealth);
PROBE(MotionController);
PROBE(MotionInput);
PROBE(MotionOutput);
PROBE(WheelPid);
PROBE(Gy511RawAxes);
PROBE(TerrainFilterQ16);
PROBE(TerrainCalibSum);
PROBE(TerrainContext);
PROBE(IrRaw);
PROBE(IrReading);
PROBE(IrTracker);
PROBE(IrTrackingOutput);
PROBE(LineIrSnapshot);
PROBE(barcode_edge_t);
PROBE(barcode_event_t);
PROBE(barcode_frame_t);
PROBE(barcode_diagnostic_t);
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
PROBE(mqtt_wheel_trace_t);
PROBE(mqtt_line_trace_t);
PROBE(mqtt_terrain_trace_t);
PROBE(comms_packed_event_t);
PROBE(comms_event_log_t);
PROBE(recovery_control_t);
PROBE(recovery_input_t);
PROBE(recovery_output_t);
PROBE(bypass_control_t);
PROBE(bypass_input_t);
PROBE(bypass_output_t);

_Static_assert(sizeof(obstacle_range_group_t) == 12u, "range group ABI");
_Static_assert(sizeof(obstacle_profile_evidence_t) == 6u, "profile evidence ABI");
_Static_assert(sizeof(obstacle_range_sample_t) == 20u, "range acquisition ABI");
/* Target-only probe: these layouts retain natural member alignment. */
_Static_assert(sizeof(MotionController) == 44u, "Motion padding regressed");
_Static_assert(sizeof(Mission) == 144u, "Mission padding regressed");
_Static_assert(sizeof(MissionInputs) == 96u, "Mission input padding regressed");
_Static_assert(sizeof(bypass_control_t) == 60u, "bypass padding regressed");
_Static_assert(sizeof(bypass_output_t) == 16u, "bypass output padding regressed");
