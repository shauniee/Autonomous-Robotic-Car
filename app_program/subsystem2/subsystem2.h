/* Mahesha: exclusive motor writer, command and stop interface. */
#ifndef SUBSYSTEM2_H
#define SUBSYSTEM2_H
#include "../app.h"
#include "motion_control.h"
#include "../common/robot_config.h"
ER subsystem2_init(void);
ER subsystem2_start(void);

/* Progress records successful output processing, not measured motion. */
typedef struct
{
    uint32_t serviced_ms, sequence;
    bool stopped;
    bool brake_acknowledged; /* Motion has braked since the latest stop. */
} MotionHealth;

/* Mission writes the command each 20 ms. Copy under a mutex; the sample time
   inside the command is never refreshed by rewriting it. */
ER s2_command(const MotionCommand *command);

/* Motion's feedback word: status (IDLE/RUNNING/DONE/FAILED) and action ID.
   Decode with rd_motion_feedback_status() / rd_motion_feedback_action(). */
uint32_t s2_feedback(void);

/* Core-1 callers only: persistent flag plus independent wake-up. */
ER s2_stop(void);
/* Fresh local action only; E_OBJ until the latest stop is serviced or after
   a startup inhibit. Clears the stored command; never commands movement. */
ER s2_reset_stop(void);
/* Any core, startup only: latches stop and a permanent inhibit. */
ER s2_inhibit_startup(void);
ER s2_health(MotionHealth *health);

#endif
