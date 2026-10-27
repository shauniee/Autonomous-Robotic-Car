/* Host test for app_program/subsystem2/motion_control.[ch] (issue #31):
   command freshness, action/status state machine, stall and invalid input. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../../app_program/subsystem2/motion_control.h"

#define PLANT_CPS_PER_DUTY 2000.0f
#define MM_PER_COUNT (3.14159265f * WHEEL_DIAMETER_MM / ENC_COUNTS_PER_REV)

typedef struct {
    MotionController ctl;
    uint32_t now, count[2];
    float frac[2];
    float wheel_gain[2];	/* 0 = wheel does not move (stalled / raised) */
} Sim;

static void sim_init(Sim *s)
{
    motion_control_reset(&s->ctl);
    s->now = 1000u;
    s->count[0] = s->count[1] = 0xFFF0u & ENC_COUNT_MASK;	/* start near the wrap */
    s->frac[0] = s->frac[1] = 0.0f;
    s->wheel_gain[0] = s->wheel_gain[1] = 1.0f;
}

/* One 20 ms release; the command's sample time is the current time unless the
   caller pins it (cmd->sampled_ms preset and keep_sample true). */
static MotionOutput sim_step(Sim *s, MotionCommand *cmd, bool keep_sample, float *duty_applied)
{
    if (!keep_sample && cmd != 0) cmd->sampled_ms = s->now;
    MotionInput in = {.now_ms = s->now, .count = {s->count[0], s->count[1]}};
    MotionOutput out;
    motion_control_step(&s->ctl, cmd, &in, &out);
    for (int w = 0; w < 2; ++w) {
        assert(fabsf(out.duty[w]) <= MOTION_MAX_DUTY + 1e-6f);
        float cps = out.duty[w] * PLANT_CPS_PER_DUTY * s->wheel_gain[w];
        s->frac[w] += cps * (MOTION_PERIOD_MS / 1000.0f);
        int whole = (int)s->frac[w];
        s->frac[w] -= (float)whole;
        s->count[w] = enc_advance(s->count[w], whole);
        if (duty_applied) duty_applied[w] = out.duty[w];
    }
    s->now += MOTION_PERIOD_MS;
    return out;
}

static void test_freshness_and_rewrite(void)
{
    Sim s; sim_init(&s);
    MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW, 100.0f, 0.0f, 0, 1u, s.now);
    assert(motion_command_fresh(&cmd, s.now + MOTION_EXPIRY_MS));
    assert(!motion_command_fresh(&cmd, s.now + MOTION_EXPIRY_MS + 1u));	/* one past the limit */
    MotionOutput out = sim_step(&s, &cmd, true, 0);
    assert(out.status == RD_MOTION_RUNNING && out.duty[0] > 0.0f);
    /* Rewriting the same command with the same sample time never refreshes it. */
    MotionCommand rewritten = cmd;
    for (int i = 0; i < 6; ++i) out = sim_step(&s, &rewritten, true, 0);
    assert(out.status == RD_MOTION_IDLE && out.duty[0] == 0.0f && out.duty[1] == 0.0f);
    /* A command stamped in the future-of-wrap is still judged by wrap-safe age. */
    cmd.sampled_ms = 0xFFFFFFF0u; s.now = 0xFFFFFFF0u + 20u;
    assert(motion_command_fresh(&cmd, s.now));
    s.now = 0x10u + 0u;				/* 32 ms after the stamp, across the wrap */
    assert(motion_command_fresh(&cmd, s.now));
    s.now = 0x40u;
    assert(!motion_command_fresh(&cmd, s.now));
}

static void test_invalid_and_stop(void)
{
    Sim s; sim_init(&s);
    /* Bad constructor inputs must fail closed. Invalid packed words still
       brake when a damaged producer bypasses the accessor. */
    MotionCommand invalid = {0};
    assert(!motion_command_set(&invalid, MOTION_MODE_FOLLOW, NAN, 0, 0, 1u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_FOLLOW, 301.0f, 0, 0, 1u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_FOLLOW, 50.0f, 0, 128, 1u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_TURN, 50.0f, 720.0f, 0, 1u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_DISTANCE, 50.0f, 2001.0f, 0, 1u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_FOLLOW, 50.0f, 0, 0, 64u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_DISTANCE, 0.0f, 100.0f, 0, 1u, 0u));
    MotionCommand bad[] = {
        {.speed_action = 301u, .mode_steering = MOTION_MODE_FOLLOW},
        {.speed_action = 50u, .mode_steering = 0xFC00u | MOTION_MODE_FOLLOW},
        {.speed_action = 50u, .mode_steering = MOTION_MODE_FOLLOW, .target = 2001},
        {.speed_action = 50u, .mode_steering = MOTION_MODE_TURN, .target = 720},
        {.speed_action = 0x8000u | 50u, .mode_steering = MOTION_MODE_FOLLOW},
        {.speed_action = 0u, .mode_steering = MOTION_MODE_DISTANCE, .target = 100},
    };
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        MotionOutput out = sim_step(&s, &bad[i], false, 0);
        assert(out.duty[0] == 0.0f && out.duty[1] == 0.0f && out.status == RD_MOTION_IDLE);
    }
    MotionOutput out = sim_step(&s, 0, false, 0);			/* no command */
    assert(out.duty[0] == 0.0f && out.status == RD_MOTION_IDLE);
    MotionCommand stop = motion_command_make(MOTION_MODE_STOP, 200.0f, 0.0f, 0, 2u, 0u);
    out = sim_step(&s, &stop, false, 0);
    assert(out.duty[0] == 0.0f && out.duty[1] == 0.0f);		/* speed field ignored */
}

static void run_to_done(Sim *s, MotionCommand *cmd, int max_steps, MotionOutput *out)
{
    for (int i = 0; i < max_steps; ++i) {
        *out = sim_step(s, cmd, false, 0);
        if (out->status != RD_MOTION_RUNNING) return;
    }
}

static void test_distance_turn_actions(void)
{
    Sim s; sim_init(&s);
    uint32_t origin_l = s.count[0], origin_r = s.count[1];
    MotionCommand cmd = motion_command_make(MOTION_MODE_DISTANCE, 200.0f, 300.0f, 0, 5u, 0u);
    MotionOutput out = {0};
    run_to_done(&s, &cmd, 2000, &out);
    assert(out.status == RD_MOTION_DONE && out.action_id == 5u);
    float travel_mm = (float)enc_travel_signed(s.count[0], origin_l) * MM_PER_COUNT;
    assert(fabsf(travel_mm - 300.0f) <= MOTION_DIST_TOL_MM + MM_PER_COUNT);
    assert(enc_travel_signed(s.count[1], origin_r) == enc_travel_signed(s.count[0], origin_l));
    /* DONE holds with the same ID and drives nothing. */
    for (int i = 0; i < 5; ++i) {
        out = sim_step(&s, &cmd, false, 0);
        assert(out.status == RD_MOTION_DONE && out.action_id == 5u && out.duty[0] == 0.0f);
    }
    /* A new action ID starts a fresh action (origin reset): reverse 100 mm. */
    cmd = motion_command_make(MOTION_MODE_DISTANCE, 200.0f, -100.0f, 0, 6u, cmd.sampled_ms);
    uint32_t reverse_origin = s.count[0];
    out = sim_step(&s, &cmd, false, 0);
    assert(out.status == RD_MOTION_RUNNING && out.action_id == 6u && out.duty[0] < 0.0f);
    run_to_done(&s, &cmd, 2000, &out);
    assert(out.status == RD_MOTION_DONE);
    travel_mm = (float)enc_travel_signed(s.count[0], reverse_origin) * MM_PER_COUNT;
    assert(fabsf(travel_mm + 100.0f) <= MOTION_DIST_TOL_MM + MM_PER_COUNT);

    /* Turn +90 degrees (left): wheels move in opposite directions by the arc. */
    uint32_t tl = s.count[0], tr = s.count[1];
    MotionCommand turn = motion_command_make(MOTION_MODE_TURN, 150.0f, 90.0f, 0, 7u, 0u);
    run_to_done(&s, &turn, 2000, &out);
    assert(out.status == RD_MOTION_DONE && out.action_id == 7u);
    float dl = (float)enc_travel_signed(s.count[0], tl) * MM_PER_COUNT;
    float dr = (float)enc_travel_signed(s.count[1], tr) * MM_PER_COUNT;
    float angle = (dr - dl) / TRACK_WIDTH_MM * (180.0f / 3.14159265f);
    assert(dl < 0.0f && dr > 0.0f);
    assert(fabsf(angle - 90.0f) <= 2.0f * MOTION_ANGLE_TOL_DEG + 2.0f);
}

static void test_stall_and_timeout(void)
{
    Sim s; sim_init(&s);
    s.wheel_gain[1] = 0.0f;				/* right wheel raised/stuck */
    MotionCommand cmd = motion_command_make(MOTION_MODE_DISTANCE, 200.0f, 500.0f, 0, 9u, 0u);
    MotionOutput out = {0};
    bool stalled = false;
    for (int i = 0; i < (int)(MOTION_STALL_MS / MOTION_PERIOD_MS) + 10 && !stalled; ++i) {
        out = sim_step(&s, &cmd, false, 0);
        stalled = out.stall;
    }
    assert(stalled && out.status == RD_MOTION_FAILED && out.duty[0] == 0.0f && out.duty[1] == 0.0f);
    out = sim_step(&s, &cmd, false, 0);			/* FAILED holds for the same ID */
    assert(out.status == RD_MOTION_FAILED && out.action_id == 9u);

    /* A stale sample during a discrete action fails it and brakes. */
    Sim t; sim_init(&t);
    MotionCommand d = motion_command_make(MOTION_MODE_DISTANCE, 100.0f, 400.0f, 0, 3u, 0u);
    out = sim_step(&t, &d, false, 0);
    assert(out.status == RD_MOTION_RUNNING);
    t.now += MOTION_EXPIRY_MS + 20u;
    out = sim_step(&t, &d, true, 0);
    assert(out.status == RD_MOTION_FAILED && out.duty[0] == 0.0f && out.duty[1] == 0.0f);
}

static void test_follow_steering(void)
{
    Sim s; sim_init(&s);
    MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW, 100.0f, 0.0f, (int)(1.0f) * 127, 4u, 0u);
    MotionOutput out = {0};
    for (int i = 0; i < 100; ++i) out = sim_step(&s, &cmd, false, 0);
    assert(out.status == RD_MOTION_RUNNING && out.duty[0] > 0.0f && out.duty[1] <= 0.02f);
    /* Zero commanded speed brakes but the status is not a stationarity proof. */
    cmd = motion_command_make(MOTION_MODE_FOLLOW, 0.0f, 0.0f, 0, 4u, cmd.sampled_ms);
    out = sim_step(&s, &cmd, false, 0);
    assert(out.duty[0] == 0.0f && out.duty[1] == 0.0f && out.status == RD_MOTION_RUNNING);
}

int main(void)
{
    test_freshness_and_rewrite();
    test_invalid_and_stop();
    test_distance_turn_actions();
    test_stall_and_timeout();
    test_follow_steering();
    puts("motion_control: PASS");
    return 0;
}
