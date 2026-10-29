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
    JUNCTION_PHASE_CROSS,
    JUNCTION_PHASE_PAUSE_WAIT,
    JUNCTION_PHASE_PAUSE,
    JUNCTION_PHASE_TURN,
    JUNCTION_PHASE_CLEAR,
    JUNCTION_PHASE_CONFIRM
} MissionJunctionPhase;

typedef enum
{
    BYPASS_STOPPING,	/* braked, waiting to be stationary */
    BYPASS_SCANNING,	/* waiting for the profile */
    BYPASS_TURN_OUT,
    BYPASS_OFFSET_OUT,
    BYPASS_TURN_PARALLEL,
    BYPASS_PASS,		/* side-watch until the range jumps */
    BYPASS_PASS_MARGIN,	/* clear the obstacle's far end */
    BYPASS_TURN_IN,
    BYPASS_OFFSET_IN,
    BYPASS_TURN_FORWARD,
    BYPASS_REACQUIRE	/* creep forward until two fresh line reads */
} MissionBypassStep;

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
    bool calibration_valid, guidance_valid, usable_line;
    IrLine line;
    int8_t line_error;		/* S3 right - left error, Q7; + means steer right */
    uint32_t line_sampled_ms;
    uint32_t travel_counts;	/* mean of both wheel counts, wrapped */
    rd_motion_status_t motion_status;
    uint8_t motion_action;
    bool stationary;		/* encoders unchanged for MISSION_STATIONARY_MS */
    bool front_valid;
    uint32_t front_age_ms;
    uint32_t front_mm;		/* front range; meaningful only if front_valid */
    bool side_valid;
    uint32_t side_mm;
    s5_scan_state_t scan_state;
    bool profile_valid;
    uint32_t left_clearance_mm, right_clearance_mm;
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
    bool scan_request;		/* ask Obstacle for a scan */
    bool scan_cancel;
    bool side_watch_on;		/* hold the servo toward the obstacle */
    int16_t side_watch_deg;
} MissionOutputs;

typedef struct
{
    MissionState state;
    MissionJunctionPhase phase;
    barcode_action_t held;	/* barcode command held for the next cross junction */
    uint32_t state_since_ms, recovery_since_ms;
    uint32_t broad_origin;	/* travel count where the current broad-black run began */
    uint32_t broad_since_ms; /* start of this broad-black encounter, wraps */
    bool in_broad;
    bool recovering;
    float turn_target;	/* degrees, + left */
    int direction;		/* last steering side: -1, 0, +1 */
    float line_integral;	/* line PID integral term, steering units */
    float line_prev_error;	/* filtered line error at line_pid_ms, -1..+1 */
    uint32_t line_pid_ms;	/* IR acquisition time of the last PID update */
    bool line_pid_primed;	/* a previous sample exists for the derivative */
    int line_steer_q;		/* PID output held until the next IR sample */
    uint32_t confirmed_sample_ms; /* last sample counted for line confirmation */
    uint32_t pause_since_ms;
    uint32_t line_seen_ms;	/* last fresh VISIBLE read while following */
    unsigned visible_streak;
    uint8_t action_id;
    uint8_t current_action;	/* ID of the Motion action in progress */
    MissionBypassStep bypass_step;
    int bypass_side;		/* +1 bypass to the left, -1 to the right */
    uint32_t bypass_origin;	/* travel count at the start of the current leg */
    uint32_t bypass_started_ms;
    uint32_t pass_baseline_mm;
    bool pass_baseline_set;
    motion_mode_t action_mode;	/* the Motion action being rewritten each release */
    float action_speed, action_target;
} Mission;

void mission_init(Mission *m, uint32_t now_ms);
void mission_step(Mission *m, const MissionInputs *in, MissionOutputs *out);

#endif /* MISSION_LOGIC_H */
