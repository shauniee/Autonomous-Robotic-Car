/* Host replay: raw DOUT bits -> ir_tracking -> snapshot -> Mission, wired as
   mission.c wires them, so no test hand-builds a convenient IR_LINE_* state.
   Scripted bits are not physics: each case checks one decision rule. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../app_program/mission/mission_logic.h"

#define TICK_MS 10u /* FOLLOW_PERIOD_MS and IR_SAMPLE_MS */
#define MM_PER_TICK 1.0f /* traces are written in mm, one per tick, whatever the speed */

typedef struct
{
    IrTracker ir;
    uint32_t now;
    float counts;
    Mission m;
    MissionInputs in;
    MissionOutputs out;
} Rig;

/* The snapshot-to-input mapping mission.c uses for the line fields. */
static void read_line(Rig *r)
{
    LineIrSnapshot s = ir_snapshot_build(&r->ir, r->now);
    r->in.now_ms = r->now;
    r->in.guidance_valid = s.guidance_valid;
    r->in.usable_line = s.normalized_valid;
    r->in.line = s.line;
    r->in.line_error = ir_snapshot_error(&s);
    r->in.left_black = s.left_black;
    r->in.right_black = s.right_black;
    r->in.line_sampled_ms = s.sampled_ms;
    uint32_t count = (uint32_t)lroundf(r->counts) & ENC_COUNT_MASK;
    r->in.wheel_count[0] = r->in.wheel_count[1] = count;
}

/* One 10 ms release; fresh=false replays the cached acquisition. */
static void tick_mm(Rig *r, unsigned left, unsigned right, bool fresh, float mm)
{
    r->now += TICK_MS;
    r->counts += mm * ENC_COUNTS_PER_MM;
    if (fresh)
    {
        ir_tracking_sample(&r->ir, (IrRaw){(uint8_t)left, (uint8_t)right}, true, r->now);
    }
    read_line(r);
    mission_step(&r->m, &r->in, &r->out);
}

static void tick(Rig *r, unsigned left, unsigned right)
{
    tick_mm(r, left, right, true, MM_PER_TICK);
}

static int steering(const Rig *r)
{
    return motion_cmd_steering(&r->out.command);
}

static void start(Rig *r)
{
    *r = (Rig){.now = 100u};
    ir_tracking_init(&r->ir);
    r->in = (MissionInputs){.inputs_ok = true, .mapped = true, .motion_status = RD_MOTION_IDLE};
    mission_init(&r->m, r->now);
    tick_mm(r, 0u, 0u, true, 0.0f);
    r->in.start_pressed = true;
    tick_mm(r, 0u, 0u, true, 0.0f);
    r->in.start_pressed = false;
    assert(r->m.state == MISSION_FOLLOW_LINE);
}

/* Following at the 100 mm/s course speed (user guardrail); steering never
   lowers it. */
static bool following(const Rig *r)
{
    return r->m.state == MISSION_FOLLOW_LINE &&
        motion_cmd_mode(&r->out.command) == MOTION_MODE_FOLLOW &&
        motion_cmd_speed(&r->out.command) == COURSE_SPEED_MMPS;
}

/* Centred on a straight, both DOUT sensors read white for as long as it
   lasts: that is good tracking, never line loss. */
static void test_centred_straight_keeps_driving(void)
{
    Rig r;
    start(&r);
    for (unsigned i = 0u; i < 500u; ++i) /* 5 s, 500 mm */
    {
        tick(&r, 0u, 0u);
        assert(following(&r) && steering(&r) == 0);
    }
}

/* After a touch, the drift makes a centred robot touch again within tens of
   mm; white/white to LINE_LOST_MM past the last touch is loss. */
static void test_lost_after_touch_enters_recovery(void)
{
    Rig r;
    start(&r);
    tick(&r, 0u, 1u);
    for (float mm = 0.0f; mm + MM_PER_TICK < LINE_LOST_MM; mm += MM_PER_TICK)
    {
        tick(&r, 0u, 0u);
        assert(following(&r) && steering(&r) == LINE_DRIFT_Q);
    }
    for (unsigned i = 0u; i < 3u && r.m.state == MISSION_FOLLOW_LINE; ++i)
    {
        tick(&r, 0u, 0u);
    }
    assert(r.m.state == MISSION_REACQUIRE);
}

/* Curve skew: the inner sensor has been black throughout and the outer one
   joins it. Keep following and keep the turn. */
static void test_curve_skew_keeps_turning(void)
{
    Rig r;
    start(&r);
    for (unsigned i = 0u; i < 40u; ++i)
    {
        tick(&r, 1u, 0u);
    }
    assert(steering(&r) < 0);
    for (unsigned i = 0u; i < 30u; ++i)
    {
        tick(&r, 1u, 1u);
        assert(following(&r) && steering(&r) < 0 && r.m.entry_curved);
    }
    mqtt_line_trace_t trace = mission_line_trace(&r.m, &r.in, &r.out);
    assert(trace.onset_skew_mm > NAV_ONSET_SKEW_MM && (trace.flags & MQTT_LINE_ENTRY_CURVED));
}

/* 2026-11-01 mechanism: a curve, a centred stretch (the integral is held through
   white/white), then a crossbar under both sensors at once. That is a
   crossing, not curve skew, whatever the integral holds. */
static void test_junction_after_curve_is_a_crossing(void)
{
    Rig r;
    start(&r);
    for (unsigned i = 0u; i < 40u; ++i)
    {
        tick(&r, 1u, 0u);
    }
    for (unsigned i = 0u; i < 20u; ++i)
    {
        tick(&r, 0u, 0u);
    }
    bool crossed = false;
    for (unsigned i = 0u; i < 23u && !crossed; ++i) /* ~23 mm measured crossing */
    {
        tick(&r, 1u, 1u);
        assert(r.m.state != MISSION_STOPPED);
        crossed = r.m.state == MISSION_NAVIGATE;
    }
    assert(crossed); /* 2026-11-01: both onsets together make a crossing */
    mqtt_line_trace_t trace = mission_line_trace(&r.m, &r.in, &r.out);
    assert(trace.sampled_ms == r.in.line_sampled_ms && trace.onset_skew_mm == 0u);
    assert((trace.flags & (MQTT_LINE_LEFT_BLACK | MQTT_LINE_RIGHT_BLACK | MQTT_LINE_ENTRY_CURVED)) ==
        (MQTT_LINE_LEFT_BLACK | MQTT_LINE_RIGHT_BLACK));
}

/* The car rolls the rest of the crossing, never stopping. The bits are what the
   sensors read as it leaves. */
static void finish_advance(Rig *r, unsigned left, unsigned right)
{
    assert(r->m.state == MISSION_NAVIGATE && r->m.phase == JUNCTION_PHASE_ADVANCE);
    for (unsigned i = 0u; i < 2u * NAV_STRAIGHT_MM && r->m.state == MISSION_NAVIGATE; ++i)
    {
        assert(motion_cmd_speed(&r->out.command) == COURSE_SPEED_MMPS);
        tick(r, left, right);
    }
    assert(r->m.state == MISSION_FOLLOW_LINE);
}

/* Drive onto a crossbar until the crossing commits. */
static void cross_bar(Rig *r)
{
    while (r->m.state != MISSION_NAVIGATE)
    {
        tick(r, 1u, 1u);
        assert(r->m.state != MISSION_STOPPED);
    }
}

/* 2026-11-04: on the car one sensor left the bar under 9 mm after both were black,
   the crossing never committed and tracking followed the other sensor off the
   line. That read now commits; a shorter both-black still does not. */
static void test_short_both_black_decides_the_crossing(void)
{
    Rig r;
    start(&r);
    for (unsigned i = 0u; i < NAV_CONFIRM_MM + 1u && r.m.state == MISSION_FOLLOW_LINE; ++i)
    {
        tick(&r, 1u, 1u);
    }
    assert(r.m.state == MISSION_NAVIGATE && motion_cmd_steering(&r.out.command) == 0);

    start(&r);
    for (unsigned i = 0u; i + 1u < NAV_CONFIRM_MM; ++i)
    {
        tick(&r, 1u, 1u);
    }
    tick(&r, 1u, 0u);
    assert(following(&r) && steering(&r) == -LINE_KP_Q);
}

/* A crossing met mid-correction read both black for only 2 mm, under
   NAV_CONFIRM_MM, and a queued LEFT went past it. A queued turn commits on two
   samples in a row; with nothing queued the same read stays a line touch.
   Navigation geometry is unreleased here, so a turn commit shows as its stop. */
static void test_queued_turn_commits_on_short_both_black(void)
{
    for (unsigned queued = 0u; queued < 2u; ++queued)
    {
        Rig r;
        start(&r);
        r.m.held = queued ? BARCODE_LEFT : BARCODE_UNMAPPED;
        for (unsigned i = 0u; i < 10u; ++i)
        {
            tick(&r, 1u, 0u); /* correcting left */
        }
        tick(&r, 1u, 1u);
        tick(&r, 1u, 1u);
        if (queued)
        {
            assert(r.m.state == MISSION_STOPPED && r.out.nav_rejected == 1u);
            continue;
        }
        tick(&r, 0u, 1u);
        assert(following(&r) && r.m.held == BARCODE_UNMAPPED);
    }
}

/* 2026-11-02: the left sensor stayed black about 12 mm past the right on most
   exits. Through the advance that is the bar, not a line touch: the command stays
   the straight advance, and the last side is kept for the drift afterwards. */
static void test_crossing_exit_ignores_trailing_sensor(void)
{
    Rig r;
    start(&r);
    tick(&r, 0u, 1u); /* last seen on the right, so the trailing left is opposite */
    tick(&r, 0u, 0u);
    cross_bar(&r);
    for (unsigned i = 0u; i < 12u; ++i)
    {
        tick(&r, 1u, 0u);
        assert(r.m.state == MISSION_NAVIGATE);
        assert(motion_cmd_mode(&r.out.command) == MOTION_MODE_FOLLOW);
        assert(motion_cmd_steering(&r.out.command) == 0);
    }
    finish_advance(&r, 0u, 0u);
    assert(following(&r) && r.m.line_side == 1 && steering(&r) == LINE_DRIFT_Q);
}

/* The crossing clears the integral: the first touch after it starts at P alone. */
static void test_first_touch_after_crossing_has_no_integral(void)
{
    Rig r;
    start(&r);
    for (unsigned i = 0u; i < 20u; ++i)
    {
        tick(&r, 0u, 1u);
    }
    assert(r.m.line_i_q == 0 && steering(&r) > LINE_KP_Q);
    tick(&r, 0u, 0u);
    assert(r.m.line_i_q > 0);
    cross_bar(&r);
    finish_advance(&r, 0u, 0u);
    assert(following(&r) && r.m.line_i_q == 0 && steering(&r) == LINE_DRIFT_Q);
    tick(&r, 0u, 1u);
    assert(following(&r) && steering(&r) == LINE_KP_Q);
}

/* 2026-11-04: regaining the line after a crossing can take over 50 mm of one touch.
   Correction grows from P with distance instead of jumping, stops at its limit,
   and fades on white/white rather than holding a lean. */
static void test_long_first_touch_after_crossing_grows_then_fades(void)
{
    Rig r;
    start(&r);
    tick(&r, 0u, 1u);
    tick(&r, 0u, 0u);
    cross_bar(&r);
    finish_advance(&r, 0u, 0u);
    int previous = 0;
    for (unsigned i = 0u; i < 2u * LINE_I_MAX_Q; ++i)
    {
        tick(&r, 1u, 0u);
        assert(following(&r) && -steering(&r) >= previous &&
            -steering(&r) <= LINE_KP_Q + LINE_I_MAX_Q);
        previous = -steering(&r);
    }
    assert(previous == LINE_KP_Q + LINE_I_MAX_Q);
    tick(&r, 0u, 0u);
    assert(following(&r) && -steering(&r) < LINE_I_MAX_Q + 1 && -steering(&r) > LINE_DRIFT_Q);
    for (unsigned i = 0u; i < 2u * LINE_I_MAX_Q; ++i)
    {
        tick(&r, 0u, 0u);
    }
    assert(following(&r) && steering(&r) == -LINE_DRIFT_Q);
}

/* Both black holds the integral; the same side afterwards resumes from it. */
static void test_both_black_freezes_integral(void)
{
    Rig r;
    start(&r);
    for (unsigned i = 0u; i < 10u; ++i)
    {
        tick(&r, 1u, 0u);
    }
    int before = -steering(&r);
    assert(before > LINE_KP_Q);
    for (unsigned i = 0u; i + 1u < NAV_CONFIRM_MM; ++i)
    {
        tick(&r, 1u, 1u); /* below the crossing distance */
    }
    tick(&r, 1u, 0u);
    assert(following(&r) && -steering(&r) >= before && -steering(&r) <= before + 2);
}

/* The other side touching clears the integral at once (S-bend). */
static void test_other_side_clears_integral(void)
{
    Rig r;
    start(&r);
    for (unsigned i = 0u; i < 2u * LINE_I_MAX_Q; ++i)
    {
        tick(&r, 1u, 0u);
    }
    assert(steering(&r) == -(LINE_KP_Q + LINE_I_MAX_Q));
    tick(&r, 0u, 1u);
    assert(following(&r) && steering(&r) == LINE_KP_Q);
}

/* A lone black still there when the advance ends is the line: track it. */
static void test_touch_past_the_bar_is_tracked(void)
{
    Rig r;
    start(&r);
    cross_bar(&r);
    finish_advance(&r, 1u, 0u);
    assert(following(&r) && steering(&r) == -LINE_KP_Q);
}

/* S-bend: one sensor, then the other. Keep driving and follow the new side
   while the second sensor is still black. */
static void test_s_bend_follows_new_side(void)
{
    Rig r;
    start(&r);
    for (unsigned i = 0u; i < 40u; ++i)
    {
        tick(&r, 1u, 0u);
        assert(following(&r));
    }
    for (unsigned i = 0u; i < 5u; ++i)
    {
        tick(&r, 0u, 0u);
        assert(following(&r));
    }
    unsigned flipped = 0u;
    for (unsigned i = 1u; i <= 60u; ++i)
    {
        tick(&r, 0u, 1u);
        assert(following(&r));
        if (!flipped && steering(&r) > 0)
        {
            flipped = i;
        }
    }
    assert(flipped == 1u); /* no integral to drain: the first touch steers */
}

/* Blank floor during the recovery sweep is not the line; a sensor crossing it
   first makes the following white/white real alignment. Recovery is driven
   with the inputs computed exactly as mission_logic.c computes them. */
static recovery_output_t recovery_tick(Rig *r, recovery_control_t *recovery,
    unsigned left, unsigned right)
{
    tick_mm(r, left, right, true, 0.0f);
    recovery_input_t input = {.now_ms = r->now, .sampled_ms = r->in.line_sampled_ms,
        .functional = r->in.usable_line, .stationary = true,
        .visible = r->in.line == IR_LINE_VISIBLE && r->in.guidance_valid,
        .black = r->in.left_black || r->in.right_black};
    recovery_output_t result;
    recovery_control_step(recovery, &input, &result);
    return result;
}

static void test_recovery_blank_floor_is_not_reacquired(void)
{
    Rig r;
    start(&r);
    recovery_control_t recovery;
    recovery_input_t begin = {.now_ms = r.now};
    recovery_control_begin(&recovery, &begin);
    for (unsigned i = 0u; i < 5u; ++i)
    {
        assert(recovery_tick(&r, &recovery, 0u, 0u).phase != RECOVERY_DONE);
    }
    assert(recovery_tick(&r, &recovery, 1u, 1u).phase != RECOVERY_DONE); /* crossing it */
    assert(recovery_tick(&r, &recovery, 0u, 0u).phase == RECOVERY_DONE);
}

/* A cached acquisition replayed by later releases is not new evidence: it
   neither grows the integral nor advances junction qualification. */
static void test_cached_samples_do_not_advance(void)
{
    Rig r;
    start(&r);
    tick(&r, 1u, 0u);
    for (unsigned i = 0u; i < 5u; ++i) /* within IR_MAX_GAP_MS */
    {
        tick_mm(&r, 1u, 0u, false, 12.0f);
        assert(following(&r) && steering(&r) == -LINE_KP_Q);
    }

    start(&r);
    tick(&r, 1u, 1u);
    assert(r.m.in_broad && !r.m.junction_qualified);
    for (unsigned i = 0u; i < 4u; ++i) /* 40 mm of travel, one sample */
    {
        tick_mm(&r, 1u, 1u, false, 10.0f);
        assert(r.m.state == MISSION_FOLLOW_LINE && !r.m.junction_qualified);
    }
}

int main(void)
{
    test_centred_straight_keeps_driving();
    test_lost_after_touch_enters_recovery();
    test_curve_skew_keeps_turning();
    test_junction_after_curve_is_a_crossing();
    test_short_both_black_decides_the_crossing();
    test_queued_turn_commits_on_short_both_black();
    test_crossing_exit_ignores_trailing_sensor();
    test_first_touch_after_crossing_has_no_integral();
    test_both_black_freezes_integral();
    test_other_side_clears_integral();
    test_long_first_touch_after_crossing_grows_then_fades();
    test_touch_past_the_bar_is_tracked();
    test_s_bend_follows_new_side();
    test_recovery_blank_floor_is_not_reacquired();
    test_cached_samples_do_not_advance();
    puts("line replay: PASS");
    return 0;
}
