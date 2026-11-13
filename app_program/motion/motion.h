/* Motion public interface: encoder arithmetic, controller, task API and
   drivers. */
#ifndef MOTION_H
#define MOTION_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"
#include "../common/robot_data.h"
#include "../comms/comms.h"

/*
 *    Kernel-free encoder arithmetic: quadrature decode and wrap-safe travel.
 *    The shared count wraps at ENC_COUNT_BITS and is never reset. Every
 *    consumer (Motion actions, barcode bar widths, hump travel, bypass legs)
 *    stores its own origin count and measures travel by modular subtraction.
 */

/* Measured rolling scale is owned by robot_config.h; track width remains
   D03. */
#define ENC_QUAD_INVALID 2    /* skipped state or both channels changed */

/* Heading in degrees from signed wheel travel counts; positive turns left. */
static inline float enc_heading_deg(int32_t left, int32_t right)
{
    return ((float)right - (float)left) / ENC_COUNTS_PER_MM /
        TRACK_WIDTH_MM * 57.2957795f;
}

/* Quadrature step between two 2-bit levels (A*2+B): +1, -1, 0 (no change),
   or ENC_QUAD_INVALID. Gray sequence 00,01,11,10 counts up. */
static inline int enc_quad_step(unsigned before, unsigned after)
{
    static const int8_t table[16] = {0, 1, -1, 2, -1, 0, 2, 1,
                                     1, 2, 0, -1, 2, -1, 1, 0};
    return (before < 4u && after < 4u)
        ? table[before * 4u + after] : ENC_QUAD_INVALID;
}

/* Leaky score of invalid edges, updated once per Motion release with the new
   edges since the last one. True when they are sustained enough to be a
   fault. */
static inline bool enc_invalid_score_update(uint32_t *score, uint32_t new_edges)
{
    *score += new_edges;
    if (0u == new_edges && *score > 0u)
    {
        --*score;
    }
    if (*score >= ENC_FAULT_INVALID_SCORE)
    {
        *score = ENC_FAULT_INVALID_SCORE;
        return true;
    }
    return false;
}

/* count + delta, wrapped to ENC_COUNT_BITS. */
/* A stored count or origin: the values wrap at ENC_COUNT_BITS. */
typedef uint16_t enc_count_t;
_Static_assert(ENC_COUNT_BITS <= 16u,
    "enc_count_t narrower than ENC_COUNT_BITS");

static inline uint32_t enc_advance(uint32_t count, int delta)
{
    return (count + (uint32_t)delta) & ENC_COUNT_MASK;
}

/* Forward travel since origin, 0..ENC_COUNT_MASK. */
static inline uint32_t enc_travel(uint32_t now, uint32_t origin)
{
    return (now - origin) & ENC_COUNT_MASK;
}

/* Signed travel since origin, in -2^(bits-1) .. 2^(bits-1)-1. */
static inline int32_t enc_travel_signed(uint32_t now, uint32_t origin)
{
    unsigned shift = 32u - ENC_COUNT_BITS;
    return (int32_t)(enc_travel(now, origin) << shift) >> shift;
}

/* Average displacements, never lifetime counts: wheels may diverge through
   repeated turns. Each wheel's displacement must stay below its half range. */
static inline float enc_mean_travel(const enc_count_t count[2],
    const enc_count_t origin[2])
{
    int64_t total = (int64_t)enc_travel_signed(count[0], origin[0]) +
        enc_travel_signed(count[1], origin[1]);
    return (float)total * 0.5f;
}

/*
 *    Kernel-free closed-loop Motion core: wheel PID on encoder speed plus
 *    distance, turn and follow actions. motion.c feeds it the latest
 *    command and counts each 20 ms release and applies the duties it returns.
 *
 *    Contract (ARCHITECTURE, Motion command contract):
 *      - A command carries the acquisition time of the sensor sample it was
 *        derived from. Rewriting a command never refreshes that time; Motion
 *        brakes when the sample is older than MOTION_EXPIRY_MS.
 *      - Discrete actions report IDLE / RUNNING / DONE / FAILED with their
 *        action ID. A command with a new action ID replaces the old action.
 *      - Zero commanded speed is never taken as proof of stopping.
 */

typedef enum
{
    MOTION_MODE_STOP,
    MOTION_MODE_FOLLOW,    /* speed_mmps, steering: continuous */
    MOTION_MODE_DISTANCE,    /* target: signed mm, speed_mmps: magnitude */
    MOTION_MODE_TURN,    /* target: signed degrees (+ left), speed_mmps:
                            magnitude */
    MOTION_MODE_COUNT
} motion_mode_t;

/* Persistent command: 10 payload bytes (12 sizeof bytes with 32-bit alignment).
   speed_action: bits 0..8 speed 0..300 mm/s, 9..14 action ID 0..63,
   bit 15 reserved. mode_steering: bits 0..1 mode 0..3, bits 2..9
   steering as signed Q7 (-127..127), bits 10..15 reserved. TURN style 2
   uses signed whole-mm path length; other styles use degrees. A target is a
   signed whole millimetre or degree, bounded by MOTION_TARGET_MAX_MM and
   +/-360 degrees respectively; sampled_ms must remain 32 bits for wrap-safe
   age checks. The sole writer constructs a complete value through accessors.
   Consumers copy under the existing mutex and never mask fields by hand. */
typedef struct
{
    uint32_t sampled_ms;
    int16_t target;
    uint16_t speed_action;
    uint16_t mode_steering;
} MotionCommand_t;

_Static_assert(sizeof(MotionCommand_t) == 12u,
    "unexpected MotionCommand_t ABI");

bool motion_command_set(MotionCommand_t *dst, motion_mode_t mode,
                        float speed_mmps, float target, int steering_q,
                        unsigned action_id, uint32_t sampled_ms);
motion_mode_t motion_cmd_mode(const MotionCommand_t *cmd);
uint16_t motion_cmd_speed(const MotionCommand_t *cmd);
int16_t motion_cmd_target(const MotionCommand_t *cmd);
int8_t motion_cmd_steering(const MotionCommand_t *cmd);
uint8_t motion_cmd_action(const MotionCommand_t *cmd);

/* A failed construction yields an all-zero STOP command; never drive with an
   invalid representation. */
MotionCommand_t motion_command_make(motion_mode_t mode, float speed_mmps,
                                  float target, int steering_q,
                                  unsigned action_id, uint32_t sampled_ms);

/* TURN uses style 0/1/3 for degree targets; style 2 is invalid (it was the
   removed smooth path). DISTANCE style 3 requests a ramped approach; FOLLOW
   retains signed Q7.
   Command size/mode packing and MQTT frames are unchanged. */
typedef enum
{
    MOTION_TURN_PIVOT,
    MOTION_TURN_SWING,
    MOTION_TURN_RAMPED = 3 /* encoder pivot with acceleration/deceleration */
} motion_turn_style_t;

/* Integral: Q4 counts, about +-2047, past what MOTION_KI can use at the duty
   cap.
   Previous speed: whole counts/s, past any wheel's reach. */
typedef struct
{
    int16_t integral_q4;
    int16_t previous_cps;
} WheelPid_t;

/* Widths of the signed fields below, stored unsigned and read with rd_sext. */
#define MOTION_ACTION_TARGET_BITS 12u
#define MOTION_PROFILE_ACCEL_BITS 11u

/* Fill the target word's spare bits and keep byte fields together. */
typedef struct
{
    uint16_t action_target : MOTION_ACTION_TARGET_BITS; /* +/-2000, rd_sext */
    uint8_t turn_style : 2; /* immutable TURN/DISTANCE style 0..3 */
    uint8_t turn_arrived : 2; /* wheels already braked inside turn tolerance */
    uint8_t mode : 2; /* motion_mode_t */
    uint8_t status : 2; /* rd_motion_status_t */
    uint8_t active : 1;
    uint8_t action_id : 6; /* RD_MOTION_ACTION_BITS */
    uint32_t start_ms;
    uint32_t previous_ms;
    enc_count_t origin[2];
    enc_count_t previous_count[2];
    uint32_t stalled_since_ms[2];
    uint16_t follow_base_q6 : 15; /* private unsigned Q6 mm/s, see specs */
    uint16_t profile_accel : MOTION_PROFILE_ACCEL_BITS; /* signed mm/s^2 */
    WheelPid_t pid[2];
    int8_t follow_steer_q; /* private applied Q7 steering, see specs */
} MotionController_t;

typedef struct
{
    uint32_t now_ms;
    enc_count_t count[2]; /* wrapped encoder counts: left, right */
} MotionInput_t;

typedef struct
{
    float duty[2];    /* signed, within +-MOTION_MAX_DUTY; 0 = brake */
    float target_cps[2], measured_cps[2]; /* local tuning diagnostics */
    uint8_t status : 2; /* rd_motion_status_t */
    uint8_t action_id : 6;
    bool stall : 1; /* stall fault condition this release */
} MotionOutput_t;

_Static_assert(MOTION_MODE_COUNT <= 4 && RD_MOTION_FAILED < 4,
    "Motion mode and status fit 2 bits");
_Static_assert(RD_MOTION_ACTION_BITS == 6u, "action IDs fit 6 bits");

/* Forget any action: next valid command starts fresh. */
/* Travel, whole mm, of a planned stop from speed_mmps. Any subsystem that plans
   around a stop uses this, so its distances stay right when the profile
   changes. */
static inline uint16_t motion_stop_distance_mm(uint16_t speed_mmps)
{
    uint32_t v = speed_mmps;
    return (uint16_t)MOTION_STOP_MM(v);
}

void motion_control_reset(MotionController_t *c);

/* True when a command may drive the wheels: valid fields and a sample no
   older than MOTION_EXPIRY_MS (wrap-safe). */
bool motion_command_fresh(const MotionCommand_t *cmd, uint32_t now_ms);

/* One release. cmd may be NULL (no command). Outputs brake duties unless a
   fresh, valid command is being executed. */
void motion_control_step(MotionController_t *c, const MotionCommand_t *cmd,
                         const MotionInput_t *in, MotionOutput_t *out);

/* Kernel-free packed Motion progress/stop snapshot, guarded by Motion mutex. */

typedef struct
{
    uint32_t serviced_ms;
    /* bit 0 stop latched, bit 1 brake acknowledged, bit 2 serviced once */
    uint8_t flags;
} MotionHealth_t;

enum
{
    MOTION_HEALTH_STOPPED_BIT = 1u,
    MOTION_HEALTH_BRAKED_BIT = 2u,
    MOTION_HEALTH_SERVICED_BIT = 4u
};
/* False until Motion has serviced its first release after boot. */
static inline bool motion_health_serviced(const MotionHealth_t *h)
{
    return (h->flags & MOTION_HEALTH_SERVICED_BIT) != 0u;
}
static inline bool motion_health_stopped(const MotionHealth_t *h)
{ return (h->flags & MOTION_HEALTH_STOPPED_BIT) != 0u; }
static inline bool motion_health_brake_acknowledged(const MotionHealth_t *h)
{ return (h->flags & MOTION_HEALTH_BRAKED_BIT) != 0u; }
_Static_assert(sizeof(MotionHealth_t) == 8u,
    "unexpected MotionHealth_t layout");

/* Kernel-free stop/reset decision shared with the real Motion service. */

static inline bool motion_stop_reset_allowed(bool inhibited, bool stopped,
    uint32_t requested, uint32_t serviced, bool cause_pending)
{
    return !inhibited && stopped && requested == serviced && !cause_pending;
}

/* Mahesha: exclusive motor writer, command and stop interface. */
ER motion_init(void);
ER motion_start(void);

/* Progress records successful output processing, not measured motion. */

/* Mission writes the command each 20 ms. Copy under a mutex; the sample time
   inside the command is never refreshed by rewriting it. */
ER motion_command(const MotionCommand_t *command);

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
ER motion_health(MotionHealth_t *health);
ER motion_health_try(MotionHealth_t *health);

/* Errors report software/BSP failures, not measured electrical output. */
ER drv_motor_init(void);
ER drv_motor_brake(void);
/* Signed duty per wheel, +forward / -backward, magnitude clamped to
   MOTION_MAX_DUTY. Non-finite input brakes and returns E_PAR. */
ER drv_motor_apply(float left, float right);

/*
 *    Quadrature decoder for both wheels, driven by the shared GPIO IRQ
 *    dispatcher on core 1. The ISR is the only writer of the counts. Counts
 *    wrap at ENC_COUNT_BITS; see motion.h for travel arithmetic.
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
