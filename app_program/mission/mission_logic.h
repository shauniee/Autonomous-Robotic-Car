/*
 *	mission_logic.h
 *	Kernel-free Mission state machine: one switch on the state with one
 *	handler per state. mission.c supplies the inputs each release and applies
 *	the outputs. The only way to move the robot is the MotionCommand returned
 *	here (the Motion contract); a state never touches motors or the encoder.
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
} MissionState;

typedef enum
{
    JUNCTION_PHASE_PAUSE_WAIT,
    JUNCTION_PHASE_PAUSE,
    JUNCTION_PHASE_TURN,
    JUNCTION_PHASE_BRAKE,
    JUNCTION_PHASE_ADVANCE
} MissionJunctionPhase;

typedef struct
{
    uint32_t now_ms;
    uint32_t line_sampled_ms;
    uint32_t wheel_count[2];
    uint32_t front_age_ms;
    uint32_t front_sampled_ms;
    uint32_t front_sequence;
    uint32_t front_wheel_count[2];
    uint32_t front_mm; /* meaningful only if front_valid */
    uint32_t side_mm;
    uint32_t side_sampled_ms;
    uint32_t side_sequence;
    uint32_t left_clearance_mm;
    uint32_t right_clearance_mm;
    uint16_t profile_closest_mm;
    uint16_t profile_planning_mm;
    uint16_t profile_width_mm;
    int16_t profile_forward_mm;
    int16_t profile_left_mm;
    bool start_pressed;		/* one debounced GP20 press this release */
    bool mapped;		/* four barcode letters installed */
    bool start_blocked;		/* a start-blocking fault bit is set */
    bool stop_down;		/* GP21 held */
    bool safety_latched;	/* Safety has latched a stop */
    bool motion_stopped;	/* Motion is in its stop latch */
    bool inputs_ok;		/* IR snapshot copied and Motion health read */
    bool guidance_valid, usable_line;
    IrLine line;
    bool left_black;
    bool right_black; /* normalized right IR; requires usable fresh sample */
    /* line_barcode right - left, Q7; + right black, steer left */
    int8_t line_error;
    rd_motion_status_t motion_status;
    uint8_t motion_action;
    bool stationary;		/* encoders unchanged for MISSION_STATIONARY_MS */
    bool front_valid;
    bool side_valid;
    obstacle_scan_state_t scan_state;
    uint8_t scan_id;
    bool profile_valid;
    uint8_t barcode_letter;	/* 0..25, 31 none */
    bool barcode_level;		/* raw GP16 level, diagnostics only */
    uint8_t barcode_edges;	/* running GP16 edge count, low 5 bits, diagnostics only */
    bool have_barcode;		/* a decoded barcode this release */
    barcode_action_t barcode_action;
} MissionInputs;

typedef struct
{
    MotionCommand command;	/* written every release; STOP unless following or acting */
    bool stop_now;		/* call motion_stop() (once, on entry to STOP) */
    uint8_t nav_rejected : 2; /* 0 none, 1 measurements, 2 geometry, 3 entry */
    bool start_refused;		/* a start press was refused: unmapped or faulted */
    bool line_lost;		/* stopped because the line could not be recovered */
    bool start_scanner;
    bool stop_scanner;
    uint8_t event_kind;		/* mqtt_event_kind_t, 0 none: one event per release */
    uint8_t event_arg;
    uint16_t event_arg2;
    uint8_t scan_id;
    bool scan_request;		/* ask Obstacle for a scan */
    bool scan_keep_narrower; /* the second sweep at this position */
    bool scan_cancel;
    bool side_watch_on;		/* hold the servo toward the obstacle */
    int16_t side_watch_deg;
} MissionOutputs;

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
    barcode_action_t held; /* barcode command held for the next junction */
    int8_t line_steer_q; /* latest digital correction target, signed Q7 */
    int8_t line_side; /* side last touched: -1 left, +1 right, 0 none */
    int8_t line_prev; /* previous acquisition's touch: -1, 0, +1 */
    int8_t cross_side; /* side last touched, kept across a straight crossing */
    int8_t line_i_q; /* integral held at last touch transition, Q7 */
    uint8_t action_id;
    uint8_t current_action;	/* ID of the Motion action in progress */
    /* low bits of the last forward reading counted after a sweep */
    uint8_t front_seen;
    uint8_t backup_mm;
    uint8_t backed_mm; /* total reversed for this obstacle */
} Mission;

bool mission_queue_direction(Mission *mission, uint8_t action);

void mission_init(Mission *m, uint32_t now_ms);
void mission_step(Mission *m, const MissionInputs *in, MissionOutputs *out);
/* True when this release's read still needs its own route event: mission_step
   reports reads only while following, and another event can replace that one. */
bool mission_barcode_event_needed(const MissionInputs *in,
    const MissionOutputs *out);
/* Diagnostic record of the line decision mission_step just made. */
mqtt_line_trace_t mission_line_trace(const Mission *m, const MissionInputs *in,
    const MissionOutputs *out);

#endif /* MISSION_LOGIC_H */
