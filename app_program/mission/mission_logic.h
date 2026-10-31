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
#include "../subsystem3/ir_tracking.h"
#include "../subsystem5/obstacle_profile.h"
#include "../subsystem3/barcode_decode.h"
#include "../subsystem2/motion_control.h"
#include "../subsystem1/mqtt_frame.h"
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
    JUNCTION_PHASE_STRAIGHT,
    JUNCTION_PHASE_BRAKE,
    JUNCTION_PHASE_ADVANCE
} MissionJunctionPhase;

typedef struct
{
    uint32_t now_ms;
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
    int8_t line_error;		/* S3 right - left, Q7; + right black, steer left */
    uint32_t line_sampled_ms;
    uint32_t wheel_count[2];
    rd_motion_status_t motion_status;
    uint8_t motion_action;
    bool hump_active; /* fresh calibrated Terrain encounter */
    bool stationary;		/* encoders unchanged for MISSION_STATIONARY_MS */
    bool front_valid;
    uint32_t front_age_ms;
    uint32_t front_sampled_ms, front_sequence, front_wheel_count[2];
    uint32_t front_mm;		/* front range; meaningful only if front_valid */
    bool side_valid;
    uint32_t side_mm, side_sampled_ms, side_sequence;
    s5_scan_state_t scan_state;
    uint8_t scan_id;
    bool profile_valid;
    uint32_t left_clearance_mm, right_clearance_mm;
    uint16_t profile_closest_mm, profile_planning_mm;
    int16_t profile_forward_mm, profile_left_mm;
    uint8_t barcode_letter;	/* 0..25, 31 none */
    bool have_barcode;		/* a decoded barcode this release */
    barcode_action_t barcode_action;
} MissionInputs;

typedef struct
{
    MotionCommand command;	/* written every release; STOP unless following or acting */
    bool stop_now;		/* call s2_stop() (once, on entry to STOP) */
    bool start_refused;		/* a start press was refused: unmapped or faulted */
    bool line_lost;		/* stopped because the line could not be recovered */
    bool start_scanner;
    bool stop_scanner;
    uint8_t event_kind;		/* mqtt_event_kind_t, 0 none: one event per release */
    uint8_t event_arg;
    uint16_t event_arg2;
    uint8_t scan_id;
    bool scan_request;		/* ask Obstacle for a scan */
    bool scan_cancel;
    bool side_watch_on;		/* hold the servo toward the obstacle */
    int16_t side_watch_deg;
} MissionOutputs;

typedef struct
{
    uint8_t state : 3;
    uint8_t phase : 3;
    barcode_action_t held;	/* barcode command held for the next cross junction */
    recovery_control_t recovery;
    uint32_t state_since_ms;
    uint32_t broad_origin[2]; /* broad entry or active navigation phase origin */
    uint32_t broad_since_ms; /* start of this broad-black encounter, wraps */
    uint8_t manual_held : 1; /* test direction protects held from barcode replacement */
    uint8_t in_broad : 1;
    uint8_t junction_open : 1;
    uint8_t junction_qualified : 1;
    uint8_t line_loss_pending : 1;
    uint32_t line_lost_since_ms; /* first LOST acquisition in this interval */
    uint32_t line_lost_sampled_ms; /* latest LOST acquisition, for continuity */
    uint32_t junction_sampled_ms;
    uint8_t turn_line_phase : 2; /* 0 exit, 1 new black, 2 white, 3 invalid */
    uint8_t turn_code : 2; /* 0 none, 1 left, 2 right, 3 U-turn */
    int8_t line_steer_q; /* latest digital correction target, signed Q7 */
    uint32_t line_steer_sampled_ms; /* last acquisition used for steering */
    uint32_t turn_sampled_ms; /* last acquisition used by the turn edge sequence */
    uint32_t pause_since_ms;
    uint8_t action_id;
    uint8_t current_action;	/* ID of the Motion action in progress */
    bypass_control_t bypass;
    uint32_t bypass_started_ms;
    uint8_t scan_phase : 1;
    uint8_t scan_retries : 1;
    uint8_t scan_id : 4;
    uint8_t reacquire_bypass : 1;

} Mission;

bool mission_queue_direction(Mission *mission, uint8_t action);

void mission_init(Mission *m, uint32_t now_ms);
void mission_step(Mission *m, const MissionInputs *in, MissionOutputs *out);

#endif /* MISSION_LOGIC_H */
