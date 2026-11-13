/*
 *    mission.h
 *    Mission task (core 1): reads the line snapshot, barcode events, buttons,
 *    Motion feedback and Safety, steps the state machine in mission_logic.c and
 *    writes one Motion command each release. It owns the GP20/GP21 button
 *    handling for the run; Safety separately latches stops.
 */

#ifndef MISSION_H
#define MISSION_H

#include "../app.h"
#include "mission_logic.h"

ER mission_start(void);

/* Current state (single load). */
MissionState_t mission_state(void);

#endif /* MISSION_H */
