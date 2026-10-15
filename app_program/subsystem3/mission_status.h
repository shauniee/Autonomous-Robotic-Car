#ifndef MISSION_STATUS_H
#define MISSION_STATUS_H
#include "barcode_types.h"
typedef enum
{
    MISSION_INITIALIZING,
    MISSION_CALIBRATING,
    MISSION_READY,
    MISSION_FOLLOW_LINE,
    MISSION_NAVIGATION,
    MISSION_SCAN,
    MISSION_BYPASS,
    MISSION_REACQUIRE,
    MISSION_STOPPED,
    MISSION_FAULT,
    MISSION_COMPLETE,
    MISSION_COMMISSIONING
} MissionState;
typedef enum
{
    REASON_NONE,
    REASON_CONFIGURATION,
    REASON_LOCAL_STOP,
    REASON_SENSOR,
    REASON_MOTION,
    REASON_BARCODE,
    REASON_AMBIGUOUS,
    REASON_BLOCKED,
    REASON_DEADLINE,
    REASON_LINE_LOST,
    REASON_COMMISSIONING
} MissionReason;
typedef enum
{
    MAP_NONE,
    MAP_PENDING,
    MAP_APPLIED,
    MAP_BUSY,
    MAP_INVALID
} MappingResult;
typedef struct
{
    uint32_t sequence, sampled_ms, run_epoch, action_id, completed_actions;
    uint32_t mapping_request_id, mapping_revision;
    MissionState state;
    MissionReason reason;
    MappingResult mapping_result;
    char symbol;
    barcode_action_t navigation;
    bool configured, moving, safety_latched;
} MissionSnapshot;
bool mission_copy(MissionSnapshot *out);
/* Exactly four assigned letters, one per motion; all others are UNMAPPED. */
bool mission_mapping_valid(const barcode_action_t mapping[BARCODE_ALPHABET]);
MappingResult
mission_mapping_submit(uint32_t request_id,
                       const barcode_action_t mapping[BARCODE_ALPHABET]);
const char *mission_state_name(MissionState state);
const char *mission_reason_name(MissionReason reason);
bool mission_junction_guard(float travel, uint32_t held_ms);
bool mission_finish_guard(uint32_t actions, float distance, uint32_t held_ms);

#endif
