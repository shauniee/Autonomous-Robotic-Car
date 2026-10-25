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
#include "ir_tracking.h"
#include "barcode_types.h"
#include "../subsystem2/motion_control.h"
#include "../common/robot_config.h"

typedef enum
{
    MISSION_CALIBRATE,
    MISSION_READY,
    MISSION_FOLLOW,
    MISSION_JUNCTION_TURN,
    MISSION_COMPLETE,
    MISSION_STOP,
    MISSION_STATE_COUNT
} MissionState;

typedef enum { JUNCTION_PHASE_TURN, JUNCTION_PHASE_CLEAR } MissionJunctionPhase;

typedef struct
{
    uint32_t now_ms;
    bool start_pressed;		/* one debounced GP20 press this release */
    bool stop_down;		/* GP21 held */
    bool safety_latched;	/* Safety has latched a stop */
    bool motion_stopped;	/* Motion is in its stop latch */
    bool inputs_ok;		/* IR snapshot copied and Motion health read */
    bool calibration_valid, guidance_valid, usable_line;
    IrLine line;
    float steering;
    uint32_t line_sampled_ms;
    uint32_t travel_counts;	/* mean of both wheel counts, wrapped */
    rd_motion_status_t motion_status;
    uint8_t motion_action;
    bool have_barcode;		/* a decoded barcode this release */
    barcode_action_t barcode_action;
} MissionInputs;

typedef struct
{
    MotionCommand command;	/* written every release; STOP unless following or acting */
    bool stop_now;		/* call s2_stop() (once, on entry to STOP) */
    bool request_reset;		/* ask Safety to release the latch (never moves) */
    bool start_scanner;
    bool stop_scanner;
} MissionOutputs;

typedef struct
{
    MissionState state;
    MissionJunctionPhase phase;
    barcode_action_t held;	/* barcode command held for the next cross junction */
    uint32_t state_since_ms, recovery_since_ms;
    uint32_t broad_origin;	/* travel count where the current broad-black run began */
    bool in_broad;
    bool recovering;
    float turn_target;	/* degrees, + left */
    bool start_pending;
    int direction;		/* last steering side: -1, 0, +1 */
    unsigned visible_streak;
    uint8_t action_id;
    uint8_t current_action;	/* ID of the Motion action in progress */
} Mission;

void mission_init(Mission *m, uint32_t now_ms);
void mission_step(Mission *m, const MissionInputs *in, MissionOutputs *out);

#endif /* MISSION_LOGIC_H */
