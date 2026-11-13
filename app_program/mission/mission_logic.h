/*
 *    mission_logic.h
 *    Kernel-free Mission state machine: one switch on the state with one
 *    handler per state. mission.c supplies the inputs each release and applies
 *    the outputs. The only way to move the robot is the MotionCommand_t
 *    returned here (the Motion contract); a state never touches motors or the
 *    encoder.
 */

#ifndef MISSION_LOGIC_H
#define MISSION_LOGIC_H

#include <stdbool.h>
#include <stdint.h>
#include "../line_barcode/line_barcode.h"
#include "../obstacle/obstacle.h"
#include "../motion/motion.h"
#include "../comms/comms.h"
#include "../common/robot_config.h"
#include "recovery_control.h"
#include "bypass_control.h"

typedef enum
{
    MISSION_READY,
    MISSION_FOLLOW_LINE,
    MISSION_NAVIGATE,
    MISSION_SCAN,
    MISSION_BYPASS,
    MISSION_REACQUIRE,
    MISSION_STOPPED,
    MISSION_STATE_COUNT
} MissionState_t;

typedef enum
{
    JUNCTION_PHASE_PAUSE_WAIT,
    JUNCTION_PHASE_PAUSE,
    JUNCTION_PHASE_TURN,
    JUNCTION_PHASE_BRAKE,
    JUNCTION_PHASE_ADVANCE
} MissionJunctionPhase_t;

/* Widths below are the minimum for each field's range (docs/team.md, data
   table). Times and sample sequences stay 32-bit for wrap-safe comparison. */
typedef struct
{
    uint32_t now_ms;
    uint32_t line_sampled_ms;
    uint32_t front_age_ms;
    uint32_t front_sampled_ms;
    uint32_t front_sequence;
    uint32_t side_sampled_ms;
    uint32_t side_sequence;
    enc_count_t wheel_count[2];
    enc_count_t front_wheel_count[2];
    uint16_t front_mm : 13; /* echo range; meaningful only if front_valid */
    uint16_t side_mm : 13;
    uint16_t left_clearance_mm : 10;
    uint16_t right_clearance_mm : 10;
    uint16_t profile_closest_mm : 10;
    uint16_t profile_planning_mm : 10;
    uint16_t profile_width_mm : 10;
    /* Signed values use the smallest signed type: BARR-C 5.3.a forbids
       signed bit-fields. */
    int16_t profile_forward_mm; /* +/-1023 mm */
    int16_t profile_left_mm;
    /* line_barcode right - left, Q7; + right black, steer left */
    int8_t line_error;
    bool start_pressed : 1; /* one debounced GP20 press this release */
    bool mapped : 1; /* four barcode letters installed */
    bool start_blocked : 1; /* a start-blocking fault bit is set */
    bool stop_down : 1; /* GP21 held */
    bool safety_latched : 1; /* Safety has latched a stop */
    bool motion_stopped : 1; /* Motion is in its stop latch */
    bool inputs_ok : 1; /* IR snapshot copied and Motion health read */
    bool guidance_valid : 1;
    bool usable_line : 1;
    bool left_black : 1;
    bool right_black : 1; /* normalized right IR; usable fresh sample only */
    bool stationary : 1; /* encoders unchanged for MISSION_STATIONARY_MS */
    bool front_valid : 1;
    bool side_valid : 1;
    bool profile_valid : 1;
    bool barcode_level : 1; /* raw GP27 level, diagnostics only */
    bool have_barcode : 1; /* a decoded barcode this release */
    uint8_t line : 2; /* IrLine_t */
    uint8_t motion_status : 2; /* rd_motion_status_t */
    uint8_t motion_action : 6; /* RD_MOTION_ACTION_BITS */
    uint8_t scan_state : 3; /* obstacle_scan_state_t */
    uint8_t scan_id : 4;
    uint8_t barcode_letter : 5; /* 0..25, 31 none */
    uint8_t barcode_edges : 5; /* running GP27 edge count, diagnostics only */
    uint8_t barcode_action : 3; /* barcode_action_t */
} MissionInputs_t;

typedef struct
{
    /* written every release; STOP unless following or acting */
    MotionCommand_t command;
    uint16_t event_arg2 : 10; /* clearance mm (<= 1023) or barcode action */
    uint8_t event_kind : 3; /* mqtt_event_kind_t, 0 none; one per release */
    uint8_t nav_rejected : 2; /* 0 none, 1 measurements, 2 geometry, 3 entry */
    uint8_t event_arg : 7; /* letter, 7-bit hump code, side or action */
    uint8_t scan_id : 4;
    int8_t side_watch_deg; /* servo angle, +/-SERVO_LOOK_DEG */
    bool stop_now : 1; /* call motion_stop() (once, on entry to STOP) */
    bool start_refused : 1; /* a start press was refused: unmapped or faulted */
    bool line_lost : 1; /* stopped because the line could not be recovered */
    bool start_scanner : 1;
    bool stop_scanner : 1;
    bool scan_request : 1; /* ask Obstacle for a scan */
    bool scan_keep_narrower : 1; /* the second sweep at this position */
    bool scan_cancel : 1;
    bool side_watch_on : 1; /* hold the servo toward the obstacle */
} MissionOutputs_t;

_Static_assert(IR_LINE_BROAD_BLACK < 4, "IrLine_t fits MissionInputs_t.line");
_Static_assert(RD_MOTION_FAILED < 4,
    "status fits MissionInputs_t.motion_status");
_Static_assert(RD_MOTION_ACTION_BITS == 6u, "motion_action follows the wire");
_Static_assert(OBSTACLE_SCAN_STATE_COUNT <= 8, "scan state fits 3 bits");
_Static_assert(BARCODE_U_TURN < 8, "barcode action fits 3 bits");
_Static_assert(MQTT_EVENT_COUNT <= 8, "event kind fits 3 bits");
_Static_assert(OBSTACLE_PROFILE_MAX_MM < 1024u, "clearance fits 10 bits");
_Static_assert(SERVO_LOOK_DEG <= INT8_MAX, "side watch angle fits int8_t");

typedef struct
{
    /* Keep aligned state first and collect small fields at the end. */
    recovery_control_t recovery;
    bypass_control_t bypass;
    uint32_t state_since_ms;
    uint32_t broad_since_ms; /* start of this broad-black encounter, wraps */
    uint32_t junction_sampled_ms;
    uint32_t onset_sampled_ms; /* last acquisition used for onsets */
    uint32_t line_steer_sampled_ms; /* last acquisition used for steering */
    uint32_t turn_sampled_ms; /* last acquisition used by the turn sequence */
    uint32_t pause_since_ms;
    uint32_t bypass_started_ms;
    enc_count_t broad_origin[2]; /* broad entry or navigation phase origin */
    enc_count_t entry_origin[2];
    enc_count_t touch_origin[2]; /* counts where the current touch began */
    enc_count_t dark_origin[2]; /* counts at the latest touching acquisition */
    enc_count_t onset[2][2]; /* counts where each sensor last went black */
    uint8_t junction_locked : 1;
    uint8_t entry_curved : 1;
    uint8_t exit_seen : 1;
    uint8_t manual_held : 1; /* test direction protects the held command */
    int16_t turn_length_mm : 12; /* signed whole mm, +/-2000 */
    uint8_t state : 3;
    uint8_t phase : 3;
    uint8_t turn_code : 2; /* 0 none, 1 left, 2 right, 3 U-turn */
    uint8_t onset_black : 2; /* black bits at the last fresh acquisition */
    uint8_t front_clear : 2;
    uint8_t in_broad : 1;
    uint8_t junction_open : 1;
    uint8_t junction_qualified : 1;
    uint8_t scan_stopping : 1; /* slowing to a profile stop before the scan */
    uint8_t scan_phase : 1;
    uint8_t scan_retries : 1;
    uint8_t scan_backing : 1;
    uint8_t scan_second : 1; /* second sweep at this position is running */
    uint8_t scan_id : 4;
    uint8_t held : 3; /* barcode_action_t held for the next junction */
    uint8_t line_i_q : 6; /* integral at last touch transition, Q7, 0..45 */
    uint8_t action_id : 6; /* last Motion action ID issued, 1..63 */
    uint8_t current_action : 6; /* ID of the Motion action in progress */
    /* -1, 0, +1: smallest signed type (BARR-C 5.3.a). */
    int8_t line_side; /* side last touched: -1 left, +1 right, 0 none */
    int8_t line_prev; /* previous acquisition's touch */
    int8_t cross_side; /* side last touched, kept across a crossing */
    int8_t line_steer_q; /* latest digital correction target, signed Q7 */
    /* low bits of the last forward reading counted after a sweep */
    uint8_t front_seen;
    uint8_t backup_mm;
    uint8_t backed_mm; /* total reversed for this obstacle */
} Mission_t;

_Static_assert((LINE_I_MAX_Q < 64) && (LINE_TURN_LEAN_Q < 64),
    "line_i_q fits 6 bits");
_Static_assert(BYPASS_BACKUP_MAX_MM <= UINT8_MAX, "backup fits 8 bits");

bool mission_queue_direction(Mission_t *mission, uint8_t action);

void mission_init(Mission_t *m, uint32_t now_ms);
void mission_step(Mission_t *m, const MissionInputs_t *in,
    MissionOutputs_t *out);
/* True when this release's read still needs its own route event: mission_step
   reports reads only while following, and another event can replace that one.
   */
bool mission_barcode_event_needed(const MissionInputs_t *in,
    const MissionOutputs_t *out);
/* Diagnostic record of the line decision mission_step just made. */
mqtt_line_trace_t mission_line_trace(const Mission_t *m,
    const MissionInputs_t *in, const MissionOutputs_t *out);

#endif /* MISSION_LOGIC_H */
