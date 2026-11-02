#ifndef MOTION_H
#define MOTION_H

#include <stdbool.h>
#include <stdint.h>
#ifndef HOST_TEST
#include "../app.h"
#endif
#include "motion_control.h"
#include "encoder_math.h"
#include "../comms/mqtt_frame.h"
#include "../common/robot_config.h"

#ifndef HOST_TEST
/* Mahesha: exclusive motor writer, command and stop interface. */
ER motion_init(void);
ER motion_start(void);

/* Progress records successful output processing, not measured motion. */

/* Mission writes the command each 20 ms. Copy under a mutex; the sample time
   inside the command is never refreshed by rewriting it. */
ER motion_command(const MotionCommand *command);

/* Motion's feedback word: status (IDLE/RUNNING/DONE/FAILED) and action ID.
   Decode with rd_motion_feedback_status() / rd_motion_feedback_action(). */
uint32_t motion_feedback(void);

/* Core-1 callers only: persistent flag plus independent wake-up. */
ER motion_stop(void);
/* Mission on eligible GP21 release only; E_OBJ while GP21/fault is active,
   until the latest stop is serviced or after
   a startup inhibit. Clears the stored command; never commands movement. */
ER motion_reset_stop(void);
/* Any core, startup only: latches stop and a permanent inhibit. */
ER motion_inhibit_startup(void);
ER motion_health(MotionHealth *health);
ER motion_health_try(MotionHealth *health);

/* Errors report software/BSP failures, not measured electrical output. */
ER drv_motor_init(void);
ER drv_motor_brake(void);
/* Signed duty per wheel, +forward / -backward, magnitude clamped to
   MOTION_MAX_DUTY. Non-finite input brakes and returns E_PAR. */
ER drv_motor_apply(float left, float right);
#endif /* !HOST_TEST */

/*
 *	motion.h
 *	Quadrature decoder for both wheels, driven by the shared GPIO IRQ
 *	dispatcher on core 1. The ISR is the only writer of the counts. Counts
 *	wrap at ENC_COUNT_BITS; see encoder_math.h for travel arithmetic.
 */

typedef enum { ENCODER_LEFT, ENCODER_RIGHT, ENCODER_WHEELS } encoder_wheel_t;

/* Core-1 task only (the dispatcher needs core 1). False if any pin could not
   be claimed. Uses GP0/GP1: build with CONSOLE=usb_cdc CRASH_REPORT=0. */
bool drv_encoder_init(void);

/* Current wrapped count: one aligned load, callable from any task or core. */
uint32_t drv_encoder_count(encoder_wheel_t wheel);

/* Number of invalid quadrature transitions since boot (ISR is the only
   writer). Motion maps a change in this value to its encoder fault bit. */
uint32_t drv_encoder_invalid_edges(void);

/* Single producer: Motion/core 1. Single consumer: I/O/core 0.
   Full drops newest; neither side waits or touches motor authority. */
bool motion_trace_publish(const mqtt_wheel_trace_t *trace);
unsigned motion_trace_take(uint8_t *out, unsigned capacity);

#endif /* MOTION_H */
