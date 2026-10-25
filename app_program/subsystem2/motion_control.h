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

typedef struct {
    motion_mode_t mode;
    float speed_mmps;	/* 0..MOTION_SPEED_MAX_MMPS */
    float steering;	/* FOLLOW only, -1..+1: left = v(1+s), right = v(1-s) */
    float target;
    uint8_t action_id;	/* fits RD_MOTION_ACTION_BITS */
    uint32_t sampled_ms;	/* acquisition time of the source sensor sample */
} MotionCommand;

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
