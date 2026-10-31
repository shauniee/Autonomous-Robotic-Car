/* Host test for app_program/subsystem2/motion_control.[ch] (issue #31):
   command freshness, action/status state machine, stall and invalid input. */
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "../../app_program/subsystem2/motion_control.h"

#define PLANT_CPS_PER_DUTY 2000.0f
#define MM_PER_COUNT (1.0f / ENC_COUNTS_PER_MM)

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
    assert(!motion_command_set(&invalid, MOTION_MODE_FOLLOW, MOTION_SPEED_MAX_MMPS + 1.0f, 0, 0, 1u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_FOLLOW, 50.0f, 0, 128, 1u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_TURN, 50.0f, 720.0f, 0, 1u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_DISTANCE, 50.0f, 2001.0f, 0, 1u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_FOLLOW, 50.0f, 0, 0, 64u, 0u));
    assert(!motion_command_set(&invalid, MOTION_MODE_DISTANCE, 0.0f, 100.0f, 0, 1u, 0u));
    MotionCommand bad[] = {
        {.speed_action = (uint16_t)(MOTION_SPEED_MAX_MMPS + 1.0f), .mode_steering = MOTION_MODE_FOLLOW},
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

static void test_action_brake_handoff(void)
{
    for (unsigned mode = MOTION_MODE_DISTANCE; mode <= MOTION_MODE_TURN; ++mode)
    {
        for (unsigned cause = 0u; cause < 3u; ++cause)
        {
            Sim s;
            sim_init(&s);
            MotionCommand cmd = motion_command_make(mode, 100.0f,
                90.0f, 0, 1u, s.now);
            MotionOutput out = sim_step(&s, &cmd, false, 0);
            assert(out.status == RD_MOTION_RUNNING);
            MotionCommand stop = motion_command_make(MOTION_MODE_STOP,
                0.0f, 0.0f, 0, 0u, s.now);
            s.now += MOTION_EXPIRY_MS + 1u;
            /* Explicit STOP, missing command, expired action respectively. */
            out = sim_step(&s, cause == 0u ? &stop :
                (cause == 1u ? 0 : &cmd), true, 0);
            assert(out.duty[0] == 0.0f && out.duty[1] == 0.0f);
            assert(out.status == (cause == 0u ?
                RD_MOTION_IDLE : RD_MOTION_FAILED));
            if (cause == 0u)
            {
                cmd = motion_command_make(mode, 100.0f,
                    mode == MOTION_MODE_TURN ? -90.0f : 90.0f,
                    0, 2u, s.now);
                out = sim_step(&s, &cmd, false, 0);
                assert(out.status == RD_MOTION_RUNNING);
            }
        }
    }
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
    if (MOTION_STALL_GUARD_ENABLED)
    {
        assert(stalled && out.status == RD_MOTION_FAILED &&
            out.duty[0] == 0.0f && out.duty[1] == 0.0f);
        out = sim_step(&s, &cmd, false, 0);
        assert(out.status == RD_MOTION_FAILED && out.action_id == 9u);
    }
    else
    {
        assert(!stalled && out.status == RD_MOTION_RUNNING);
        assert(out.duty[1] > 0.0f);
        s.now += MOTION_EXPIRY_MS + 1u;
        out = sim_step(&s, &cmd, true, 0);
        assert(out.status == RD_MOTION_FAILED &&
            out.duty[0] == 0.0f && out.duty[1] == 0.0f);
    }

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
    const int steering[] = {-127, -96, -64, 0, 64, 96, 127};
    const float targets[][2] = {
        {0.0f, 32.0f}, {9.841270f, 29.070866f}, {20.0f, 26.047244f},
        {20.0f, 20.0f},
        {26.047244f, 20.0f}, {29.070866f, 9.841270f}, {32.0f, 0.0f}
    };
    Sim s;
    MotionCommand cmd;
    MotionOutput out = {0};
    for (unsigned i = 0; i < sizeof(steering) / sizeof(steering[0]); ++i)
    {
        sim_init(&s);
        cmd = motion_command_make(MOTION_MODE_FOLLOW, 20.0f, 0.0f,
            steering[i], 4u, 0u);
        out = sim_step(&s, &cmd, false, 0);
        assert(out.status == RD_MOTION_RUNNING);
        /* Verify the mixer independently of the steering ramp. */
        sim_init(&s);
        s.ctl.active = true;
        s.ctl.action_id = 4u;
        s.ctl.mode = MOTION_MODE_FOLLOW;
        s.ctl.status = RD_MOTION_RUNNING;
        s.ctl.previous_ms = s.now - MOTION_PERIOD_MS;
        s.ctl.previous_count[0] = s.count[0];
        s.ctl.previous_count[1] = s.count[1];
        s.ctl.stalled_since_ms[0] = s.now;
        s.ctl.stalled_since_ms[1] = s.now;
        s.ctl.follow_steer_q = (int8_t)steering[i];
        out = sim_step(&s, &cmd, false, 0);
        for (unsigned w = 0; w < 2u; ++w)
        {
            float initial_base = fminf(20.0f,
                MOTION_FOLLOW_SLEW_MMPS2 * MOTION_PERIOD_MS / 1000.0f);
            float initial_mmps = targets[i][w] * initial_base / 20.0f;
            float cps = initial_mmps * ENC_COUNTS_PER_MM;
            float expected = cps == 0.0f ? 0.0f : MOTION_KS + cps * (MOTION_KF + MOTION_KP +
                MOTION_KI * ((float)MOTION_PERIOD_MS / 1000.0f));
            assert(expected < MOTION_MAX_DUTY);
            assert(fabsf(out.duty[w] - expected) < 0.00001f);
        }
        for (unsigned release = 0; release < 8u; ++release)
        {
            out = sim_step(&s, &cmd, false, 0);
            assert(out.status == RD_MOTION_RUNNING);
        }
        assert(s.ctl.follow_base_q6 == 1280u);
    }
    puts("INT-85: FOLLOW base ramp preserves gentle/tight wheel ratios from rest PASS");
    /* Zero commanded speed brakes but the status is not a stationarity proof. */
    cmd = motion_command_make(MOTION_MODE_FOLLOW, 0.0f, 0.0f, 0, 4u, cmd.sampled_ms);
    out = sim_step(&s, &cmd, false, 0);
    assert(out.duty[0] == 0.0f && out.duty[1] == 0.0f && out.status == RD_MOTION_RUNNING);
}

static void test_follow_target_slew(void)
{
    MotionController c;
    motion_control_reset(&c);
    MotionInput in = {.now_ms = UINT32_MAX - 99u};
    MotionOutput out;
    const int steering[] = {0, -127, 127, 0};
    for (unsigned phase = 0; phase < 4u; ++phase)
    {
        for (unsigned i = 0; i < 40u; ++i)
        {
            unsigned before = c.follow_base_q6;
            MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW,
                75.0f, 0.0f, steering[phase], 1u, in.now_ms);
            motion_control_step(&c, &cmd, &in, &out);
            assert(c.follow_base_q6 >= before);
            assert(c.follow_base_q6 <= before + 320u);
            assert(c.follow_base_q6 <= 4800u);
            assert(out.duty[0] >= 0.0f && out.duty[1] >= 0.0f);
            unsigned held = c.follow_base_q6;
            motion_control_step(&c, &cmd, &in, &out);
            assert(c.follow_base_q6 == held);
            in.now_ms += MOTION_PERIOD_MS;
            in.count[0] = enc_advance(in.count[0], 12);
            in.count[1] = enc_advance(in.count[1], 12);
        }
        assert(c.follow_base_q6 == 4800u);
    }
    /* Lower base demands ramp down, with no reset on steering reversal. */
    for (unsigned i = 0; i < 12u; ++i)
    {
        unsigned before = c.follow_base_q6;
        MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW,
            20.0f, 0.0f, -127, 1u, in.now_ms);
        motion_control_step(&c, &cmd, &in, &out);
        assert(c.follow_base_q6 <= before && c.follow_base_q6 >= 1280u);
        assert(before - c.follow_base_q6 <= 320u);
        in.now_ms += MOTION_PERIOD_MS;
        in.count[0] = enc_advance(in.count[0], 12);
        in.count[1] = enc_advance(in.count[1], 12);
    }
    assert(c.follow_base_q6 == 1280u);
    /* Zero demand, stale/invalid commands and STOP bypass the slew limit. */
    for (unsigned cause = 0; cause < 5u; ++cause)
    {
        MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW,
            75.0f, 0.0f, 127, 1u, in.now_ms);
        motion_control_step(&c, &cmd, &in, &out);
        if (cause == 0u)
        {
            cmd = motion_command_make(MOTION_MODE_FOLLOW, 0.0f, 0.0f,
                0, 1u, in.now_ms);
        }
        else if (cause == 1u)
        {
            in.now_ms += MOTION_EXPIRY_MS + 1u;
        }
        else if (cause == 2u)
        {
            cmd.mode_steering |= 0x8000u;
        }
        else if (cause == 3u)
        {
            cmd = motion_command_make(MOTION_MODE_STOP, 0.0f, 0.0f,
                0, 1u, in.now_ms);
        }
        else
        {
            c.follow_base_q6 = 32767u;
        }
        motion_control_step(&c, &cmd, &in, &out);
        assert(out.duty[0] == 0.0f && out.duty[1] == 0.0f);
        assert(c.follow_base_q6 == 0u);
        in.now_ms += MOTION_PERIOD_MS;
    }
    /* A late release cannot catch up with a large speed jump. */
    MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW, 75.0f,
        0.0f, 127, 1u, in.now_ms);
    motion_control_step(&c, &cmd, &in, &out);
    unsigned before = c.follow_base_q6;
    in.now_ms += 100u;
    cmd.sampled_ms = in.now_ms;
    motion_control_step(&c, &cmd, &in, &out);
    assert(c.follow_base_q6 == before + 320u);
    motion_control_reset(&c);
    cmd = motion_command_make(MOTION_MODE_FOLLOW, 1.0f, 0.0f,
        0, 1u, in.now_ms);
    motion_control_step(&c, &cmd, &in, &out);
    assert(c.follow_base_q6 == 64u);
    for (unsigned i = 0; i < 105u; ++i)
    {
        in.now_ms += MOTION_PERIOD_MS;
        in.count[0] = enc_advance(in.count[0], 12);
        in.count[1] = enc_advance(in.count[1], 12);
        cmd = motion_command_make(MOTION_MODE_FOLLOW, 500.0f,
            0.0f, 127, 1u, in.now_ms);
        motion_control_step(&c, &cmd, &in, &out);
        assert(out.status == RD_MOTION_RUNNING);
        assert(c.follow_base_q6 <= 32000u);
    }
    assert(c.follow_base_q6 == 32000u);
    c.follow_base_q6 = 32001u;
    motion_control_step(&c, &cmd, &in, &out);
    assert(out.status == RD_MOTION_FAILED);
    assert(out.duty[0] == 0.0f && out.duty[1] == 0.0f);
    assert(c.follow_base_q6 == 0u);
    puts("INT-85: common base slew, reversal, wrap, duplicates, bounds and immediate brakes PASS");
}

static void test_follow_steering_slew(void)
{
    MotionController c;
    motion_control_reset(&c);
    MotionInput in = {.now_ms = UINT32_MAX - 39u};
    MotionOutput out;
    /* Entry, full reversal through zero, and return to straight. */
    const int targets[] = {127, -127, 0};
    const unsigned releases[] = {8u, 16u, 8u};
    for (unsigned phase = 0; phase < 3u; ++phase)
    {
        for (unsigned i = 0; i < releases[phase]; ++i)
        {
            int previous = c.follow_steer_q;
            MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW,
                20.0f, 0.0f, targets[phase], 1u, in.now_ms);
            motion_control_step(&c, &cmd, &in, &out);
            assert(out.status == RD_MOTION_RUNNING);
            int delta = c.follow_steer_q - previous;
            assert(delta >= -16 && delta <= 16);
            assert(c.follow_steer_q >= -127 && c.follow_steer_q <= 127);
            int held = c.follow_steer_q;
            motion_control_step(&c, &cmd, &in, &out);
            assert(c.follow_steer_q == held); /* duplicate time */
            if (i + 1u == releases[phase] && phase != 2u)
            {
                assert(out.duty[phase == 0u ? 1 : 0] == 0.0f);
                assert(out.duty[phase == 0u ? 0 : 1] > 0.0f);
            }
            in.now_ms += MOTION_PERIOD_MS;
            in.count[0] = enc_advance(in.count[0], 1);
            in.count[1] = enc_advance(in.count[1], 1);
        }
        assert(c.follow_steer_q == targets[phase]);
    }
    /* A single black acquisition followed by white is a small correction. */
    MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW,
        20.0f, 0.0f, -127, 1u, in.now_ms);
    motion_control_step(&c, &cmd, &in, &out);
    assert(c.follow_steer_q == -16);
    in.now_ms += MOTION_PERIOD_MS;
    cmd = motion_command_make(MOTION_MODE_FOLLOW, 20.0f, 0.0f, 0, 1u, in.now_ms);
    motion_control_step(&c, &cmd, &in, &out);
    assert(c.follow_steer_q == 0);
    /* Late release progress is capped; expired samples still stop immediately. */
    in.now_ms += 100u;
    cmd = motion_command_make(MOTION_MODE_FOLLOW, 20.0f, 0.0f, 127, 1u, in.now_ms);
    motion_control_step(&c, &cmd, &in, &out);
    assert(c.follow_steer_q == 16);
    for (unsigned cause = 0; cause < 6u; ++cause)
    {
        in.now_ms += MOTION_PERIOD_MS;
        cmd = motion_command_make(MOTION_MODE_FOLLOW, 20.0f, 0.0f, 127, 1u, in.now_ms);
        motion_control_step(&c, &cmd, &in, &out);
        assert(c.follow_steer_q > 0);
        if (cause == 0u) cmd = motion_command_make(MOTION_MODE_STOP, 0, 0, 0, 0, in.now_ms);
        else if (cause == 1u) cmd = motion_command_make(MOTION_MODE_FOLLOW, 0, 0, 127, 1, in.now_ms);
        else if (cause == 2u) in.now_ms += MOTION_EXPIRY_MS + 1u;
        else if (cause == 3u) cmd.mode_steering |= 0x8000u;
        else if (cause == 4u) c.follow_steer_q = INT8_MIN;
        else motion_control_reset(&c);
        motion_control_step(&c, cause == 5u ? NULL : &cmd, &in, &out);
        assert(c.follow_steer_q == 0 && c.follow_base_q6 == 0u);
        assert(out.duty[0] == 0.0f && out.duty[1] == 0.0f);
    }
    puts("INT-92: applied steering slew, pivot, chatter, reversal, wrap and immediate brakes PASS");
}

static void test_follow_slowdown_never_reverses(void)
{
    /* Tracking -> recovery keeps FOLLOW action 1 but sharply reduces speed.
       Inject forward encoder travel from the preceding faster command. */
    MotionController control;
    motion_control_reset(&control);
    MotionInput in = {.now_ms = 1000u, .count = {100u, 100u}};
    MotionOutput out;
    MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW,
        MISSION_FOLLOW_SPEED_MMPS, 0.0f, 0, 1u, in.now_ms);
    motion_control_step(&control, &cmd, &in, &out);
    in.now_ms += MOTION_PERIOD_MS;
    in.count[0] += 16u;
    in.count[1] += 16u;
    /* Preserve the old low-speed count target after correcting its mm label. */
    cmd = motion_command_make(MOTION_MODE_FOLLOW, floorf(MISSION_BROAD_SPEED_MMPS),
        0.0f, -96, 1u, in.now_ms);
    motion_control_step(&control, &cmd, &in, &out);
    printf("tracking -> recovery slowdown: left duty=%f right duty=%f\n",
        (double)out.duty[0], (double)out.duty[1]);
    fflush(stdout);
    assert(out.status == RD_MOTION_RUNNING && !out.stall);
    assert(out.duty[0] >= 0.0f && out.duty[1] >= 0.0f);
    /* The saturated controller must recover rather than winding below zero. */
    for (unsigned i = 0u; i < 5u; ++i)
    {
        in.now_ms += MOTION_PERIOD_MS;
        cmd.sampled_ms = in.now_ms;
        motion_control_step(&control, &cmd, &in, &out);
        assert(out.duty[0] >= 0.0f && out.duty[1] >= 0.0f);
    }
    assert(out.duty[0] > 0.0f && out.duty[1] > 0.0f);
}

static void test_discrete_overspeed_does_not_reverse(void)
{
    for (unsigned mode = MOTION_MODE_DISTANCE; mode <= MOTION_MODE_TURN; ++mode)
    {
        for (int direction = -1; direction <= 1; direction += 2)
        {
            MotionController control;
            motion_control_reset(&control);
            MotionInput in = {.now_ms = 100u, .count = {0u, 0u}};
            float target = direction * (mode == MOTION_MODE_DISTANCE ? 28.0f : 90.0f);
            MotionCommand cmd = motion_command_make((motion_mode_t)mode,
                floorf(MISSION_BROAD_SPEED_MMPS), target, 0, 2u, in.now_ms);
            MotionOutput out;
            motion_control_step(&control, &cmd, &in, &out);
            float goal = mode == MOTION_MODE_DISTANCE ? 28.0f * ENC_COUNTS_PER_MM :
                TRACK_WIDTH_MM * 0.5f * 90.0f * 3.14159265f / 180.0f * ENC_COUNTS_PER_MM;
            for (unsigned travel = 16u; travel + 6u < goal; travel += 16u)
            {
                int left_sign = mode == MOTION_MODE_TURN ? -direction : direction;
                int right_sign = direction;
                in.count[0] = enc_advance(0u, left_sign * (int)travel);
                in.count[1] = enc_advance(0u, right_sign * (int)travel);
                in.now_ms += MOTION_PERIOD_MS;
                cmd.sampled_ms = in.now_ms;
                motion_control_step(&control, &cmd, &in, &out);
                if (out.duty[0] * left_sign < 0.0f || out.duty[1] * right_sign < 0.0f)
                {
                    printf("INT-37 reproduction: mode=%u target=%g travel=%u duty=%f/%f\n",
                        mode, (double)target, travel, (double)out.duty[0], (double)out.duty[1]);
                    fflush(stdout);
                }
                assert(out.duty[0] * left_sign >= 0.0f);
                assert(out.duty[1] * right_sign >= 0.0f);
            }
        }
    }
    /* Position overshoot is distinct from speed overshoot: allow corrective
       reverse wheel targets after passing the distance goal. */
    MotionController control;
    motion_control_reset(&control);
    MotionInput in = {.now_ms = 100u, .count = {0u, 0u}};
    MotionCommand cmd = motion_command_make(MOTION_MODE_DISTANCE, 90.0f,
        28.0f, 0, 3u, in.now_ms);
    MotionOutput out;
    motion_control_step(&control, &cmd, &in, &out);
    in.count[0] = in.count[1] = (uint32_t)(40.0f * ENC_COUNTS_PER_MM);
    in.now_ms += MOTION_PERIOD_MS;
    cmd.sampled_ms = in.now_ms;
    motion_control_step(&control, &cmd, &in, &out);
    assert(out.status == RD_MOTION_RUNNING);
    assert(out.duty[0] < 0.0f && out.duty[1] < 0.0f);
    puts("INT-37: discrete overspeed direction clamp and position correction PASS");
}

static void test_turn_endpoint_does_not_reactivate(void)
{
    for (int direction = -1; direction <= 1; direction += 2)
    {
        MotionController control;
        motion_control_reset(&control);
        MotionInput in = {.now_ms = 100u, .count = {0u, 0u}};
        MotionCommand cmd = motion_command_make(MOTION_MODE_TURN, 100.0f,
            direction * 90.0f, 0, 4u, in.now_ms);
        MotionOutput out;
        motion_control_step(&control, &cmd, &in, &out);
        int arc = (int)lroundf(TRACK_WIDTH_MM * 0.5f * 90.0f *
            3.14159265f / 180.0f * ENC_COUNTS_PER_MM);
        in.count[0] = enc_advance(0u, -direction * arc);
        in.now_ms += MOTION_PERIOD_MS;
        cmd.sampled_ms = in.now_ms;
        motion_control_step(&control, &cmd, &in, &out);
        assert(out.status == RD_MOTION_RUNNING && out.duty[0] == 0.0f);
        int excursion = (int)ceilf(TRACK_WIDTH_MM * 0.5f *
            MOTION_ANGLE_TOL_DEG * 3.14159265f / 180.0f * ENC_COUNTS_PER_MM) + 12;
        for (int drift = -excursion; drift <= excursion; drift += 2 * excursion)
        {
            in.count[0] = enc_advance(0u, -direction * (arc + drift));
            in.now_ms += MOTION_PERIOD_MS;
            cmd.sampled_ms = in.now_ms;
            motion_control_step(&control, &cmd, &in, &out);
            printf("INT-38 endpoint: direction=%d drift=%d left_duty=%f\n",
                direction, drift, (double)out.duty[0]);
            fflush(stdout);
            assert(out.duty[0] == 0.0f);
        }
        /* Partner arrival must not hide drift outside tolerance. */
        in.count[1] = enc_advance(0u, direction * arc);
        in.now_ms += MOTION_PERIOD_MS;
        cmd.sampled_ms = in.now_ms;
        motion_control_step(&control, &cmd, &in, &out);
        assert(out.status == RD_MOTION_RUNNING);
        assert(out.duty[0] == 0.0f && out.duty[1] == 0.0f);
        in.now_ms = 100u + MOTION_ACTION_TIMEOUT_MS;
        cmd.sampled_ms = in.now_ms;
        motion_control_step(&control, &cmd, &in, &out);
        assert(out.status == RD_MOTION_FAILED);
        /* A new action is permitted to drive the same wheel again. */
        cmd = motion_command_make(MOTION_MODE_TURN, 100.0f,
            direction * 90.0f, 0, 5u, in.now_ms);
        motion_control_step(&control, &cmd, &in, &out);
        assert(out.status == RD_MOTION_RUNNING && out.duty[0] != 0.0f);
    }
}

static void test_swing_turn(void)
{
    for (int direction = -1; direction <= 1; direction += 2)
    {
        Sim s;
        sim_init(&s);
        MotionCommand cmd = motion_command_make(MOTION_MODE_TURN, 100.0f,
            direction * 90.0f, MOTION_TURN_SWING, 2u, s.now);
        unsigned driven = direction > 0 ? 1u : 0u;
        unsigned braked = 1u - driven;
        uint32_t origin = s.count[driven];
        MotionOutput out;
        for (unsigned i = 0; i < 1000u; ++i)
        {
            out = sim_step(&s, &cmd, false, 0);
            assert(out.duty[braked] == 0.0f);
            assert(out.duty[driven] >= 0.0f);
            /* Drift of the braked wheel must never energize it. */
            s.count[braked] = enc_advance(s.count[braked], 1);
            if (out.status != RD_MOTION_RUNNING)
            {
                break;
            }
        }
        assert(out.status == RD_MOTION_DONE);
        float travelled = enc_travel_signed(s.count[driven], origin) / ENC_COUNTS_PER_MM;
        float expected = TRACK_WIDTH_MM * 3.14159265f * 0.5f;
        float tolerance = TRACK_WIDTH_MM * MOTION_ANGLE_TOL_DEG *
            3.14159265f / 180.0f + MM_PER_COUNT;
        assert(fabsf(travelled - expected) <= tolerance);
    }
    MotionCommand cmd;
    assert(!motion_command_set(&cmd, MOTION_MODE_TURN, 50.0f,
        90.0f, -1, 2u, 100u));
    assert(!motion_command_set(&cmd, MOTION_MODE_TURN, 50.0f,
        90.0f, 2, 2u, 100u));
    cmd = motion_command_make(MOTION_MODE_TURN, 50.0f, 90.0f,
        MOTION_TURN_PIVOT, 2u, 100u);
    cmd.mode_steering |= 2u << 2u;
    assert(!motion_command_fresh(&cmd, 100u));
    puts("INT-84: forward single-wheel swing, geometry, braked-wheel drift and invalid style PASS");
}

static void test_wheel_trace_inputs(void)
{
    MotionController c;
    motion_control_reset(&c);
    MotionInput in = {.now_ms = 100u, .count = {65530u, 2u}};
    MotionOutput out;
    MotionCommand cmd = motion_command_make(MOTION_MODE_FOLLOW,
        50.0f, 0.0f, 0, 1u, in.now_ms);
    motion_control_step(&c, &cmd, &in, &out);
    assert(isnan(out.measured_cps[0]) && isnan(out.measured_cps[1]));
    assert(fabsf(out.target_cps[0] - 5.0f * ENC_COUNTS_PER_MM) < 0.01f);
    in.now_ms += 20u;
    in.count[0] = enc_advance(in.count[0], 20);
    in.count[1] = enc_advance(in.count[1], -10);
    cmd.sampled_ms = in.now_ms;
    motion_control_step(&c, &cmd, &in, &out);
    assert(fabsf(out.measured_cps[0] - 1000.0f) < 0.01f);
    assert(fabsf(out.measured_cps[1] + 500.0f) < 0.01f);
    assert(fabsf(out.target_cps[1] - 10.0f * ENC_COUNTS_PER_MM) < 0.01f);
    in.now_ms += MOTION_EXPIRY_MS + 1u;
    motion_control_step(&c, &cmd, &in, &out);
    assert(out.target_cps[0] == 0.0f && out.target_cps[1] == 0.0f);
    assert(out.duty[0] == 0.0f && out.duty[1] == 0.0f);
    assert(isnan(out.measured_cps[0]));
}

int main(void)
{
    test_wheel_trace_inputs();
    test_freshness_and_rewrite();
    test_invalid_and_stop();
    test_action_brake_handoff();
    test_distance_turn_actions();
    test_swing_turn();
    test_stall_and_timeout();
    test_follow_steering();
    test_follow_target_slew();
    test_follow_steering_slew();
    test_follow_slowdown_never_reverses();
    test_discrete_overspeed_does_not_reverse();
    test_turn_endpoint_does_not_reactivate();
    puts("motion_control: PASS");
    return 0;
}
