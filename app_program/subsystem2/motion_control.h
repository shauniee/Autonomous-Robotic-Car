/*
 *	motion_control.h
 *	Kernel-free closed-loop Motion core: wheel PID on encoder speed plus
 *	distance, turn and follow actions. subsystem2.c feeds it the latest
 *	command and counts each 20 ms release and applies the duties it returns.
 *
 *	Contract (ARCHITECTURE, Motion command contract):
 *	  - A command carries the acquisition time of the sensor sample it was
 *	    derived from. Rewriting a command never refreshes that time; Motion
 *	    brakes when the sample is older than MOTION_EXPIRY_MS.
 *	  - Discrete actions report IDLE / RUNNING / DONE / FAILED with their
 *	    action ID. A command with a new action ID replaces the old action.
 *	  - Zero commanded speed is never taken as proof of stopping.
 */

#ifndef MOTION_CONTROL_H
#define MOTION_CONTROL_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_data.h"
#include "../common/robot_config.h"
#include "encoder_math.h"

typedef enum {
    MOTION_MODE_STOP,
    MOTION_MODE_FOLLOW,	/* speed_mmps, steering: continuous */
    MOTION_MODE_DISTANCE,	/* target: signed mm, speed_mmps: magnitude */
    MOTION_MODE_TURN,	/* target: signed degrees (+ left), speed_mmps: magnitude */
    MOTION_MODE_COUNT
} motion_mode_t;

/* Persistent command: 10 payload bytes (12 sizeof bytes with 32-bit alignment).
   speed_action: bits 0..8 speed 0..300 mm/s, 9..14 action ID 0..63,
   bit 15 reserved. mode_steering: bits 0..1 mode 0..3, bits 2..9
   steering as signed Q7 (-127..127), bits 10..15 reserved. A target is a
   signed whole millimetre or degree, bounded by MOTION_TARGET_MAX_MM and
   +/-360 degrees respectively; sampled_ms must remain 32 bits for wrap-safe
   age checks. The sole writer constructs a complete value through accessors.
   Consumers copy under the existing mutex and never mask fields by hand. */
typedef struct {
    uint32_t sampled_ms;
    int16_t target;
    uint16_t speed_action;
    uint16_t mode_steering;
} MotionCommand;

_Static_assert(sizeof(MotionCommand) == 12u, "unexpected MotionCommand ABI");

bool motion_command_set(MotionCommand *dst, motion_mode_t mode, float speed_mmps,
                        float target, int steering_q, unsigned action_id, uint32_t sampled_ms);
motion_mode_t motion_cmd_mode(const MotionCommand *cmd);
uint16_t motion_cmd_speed(const MotionCommand *cmd);
int16_t motion_cmd_target(const MotionCommand *cmd);
int8_t motion_cmd_steering(const MotionCommand *cmd);
uint8_t motion_cmd_action(const MotionCommand *cmd);

/* A failed construction yields an all-zero STOP command; never drive with an
   invalid representation. */
MotionCommand motion_command_make(motion_mode_t mode, float speed_mmps, float target,
                                  int steering_q, unsigned action_id, uint32_t sampled_ms);

typedef struct {
    float integral, previous_speed;
} WheelPid;

typedef struct {
    bool active;
    uint8_t action_id;
    motion_mode_t mode;
    rd_motion_status_t status;
    uint32_t start_ms, previous_ms;
    uint32_t origin[2];
    uint32_t previous_count[2];
    uint32_t stalled_since_ms[2];
    WheelPid pid[2];
} MotionController;

typedef struct {
    uint32_t now_ms;
    uint32_t count[2];	/* wrapped encoder counts: left, right */
} MotionInput;

typedef struct {
    float duty[2];	/* signed, within +-MOTION_MAX_DUTY; 0 = brake */
    rd_motion_status_t status;
    uint8_t action_id;
    bool stall;		/* stall fault condition this release */
} MotionOutput;

/* Forget any action: next valid command starts fresh. */
void motion_control_reset(MotionController *c);

/* True when a command may drive the wheels: valid fields and a sample no
   older than MOTION_EXPIRY_MS (wrap-safe). */
bool motion_command_fresh(const MotionCommand *cmd, uint32_t now_ms);

/* One release. cmd may be NULL (no command). Outputs brake duties unless a
   fresh, valid command is being executed. */
void motion_control_step(MotionController *c, const MotionCommand *cmd,
                         const MotionInput *in, MotionOutput *out);

#endif /* MOTION_CONTROL_H */
