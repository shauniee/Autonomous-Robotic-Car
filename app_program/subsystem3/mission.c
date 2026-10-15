#include "mission.h"
#include "subsystem3.h"
#include "barcode_adapter.h"
#include "../subsystem2/subsystem2.h"
#include "../subsystem4/subsystem4.h"
#include "../subsystem5/subsystem5.h"
#include "../common/drv_buttons.h"
#include "../common/button.h"
#include "../common/robot_time.h"
#include "../common/robot_config.h"
#include <stdatomic.h>
#include <math.h>
#include <string.h>

static atomic_int snapshot_mutex;
static atomic_bool released, moving, safety_latched;
static atomic_uint safety_reason, mission_progress, current_mode;
static MissionSnapshot latest;
static barcode_action_t pending_mapping[BARCODE_ALPHABET];
static uint32_t pending_id;
static bool mapping_pending;

const char *mission_state_name(MissionState state)
{
    static const char *const names[] = {"INITIALIZING", "CALIBRATING", "READY",
        "FOLLOW_LINE", "NAVIGATION", "SCAN", "BYPASS", "REACQUIRE",
        "STOPPED", "FAULT", "COMPLETE"};
    return (unsigned)state < sizeof(names) / sizeof(names[0]) ? names[state] : "FAULT";
}
const char *mission_reason_name(MissionReason reason)
{
    static const char *const names[] = {"none", "configuration_pending", "local_stop",
        "required_sensor", "motion_fault", "barcode_fault", "ambiguous_route",
        "blocked_path", "deadline", "line_lost"};
    return (unsigned)reason < sizeof(names) / sizeof(names[0]) ? names[reason] : "fault";
}
bool mission_copy(MissionSnapshot *out)
{
    ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
    if (out == NULL || id <= 0 || tk_loc_mtx(id, TMO_POL) < E_OK) { return false; }
    *out = latest;
    return tk_unl_mtx(id) >= E_OK;
}
MappingResult mission_mapping_submit(uint32_t request_id,
    const barcode_action_t mapping[BARCODE_ALPHABET])
{
    if (mapping == NULL || request_id == 0u) { return MAP_INVALID; }
    for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
    {
        if ((unsigned)mapping[i] > BARCODE_U_TURN) { return MAP_INVALID; }
    }
    ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
    if (id <= 0 || tk_loc_mtx(id, TMO_POL) < E_OK) { return MAP_BUSY; }
    MappingResult result = MAP_BUSY;
    if (!mapping_pending && !latest.moving && latest.state != MISSION_SCAN)
    {
        memcpy(pending_mapping, mapping, sizeof(pending_mapping));
        pending_id = request_id;
        mapping_pending = true;
        result = MAP_PENDING;
    }
    (void)tk_unl_mtx(id);
    return result;
}
bool mission_junction_guard(float distance, uint32_t held_ms)
{
    return robot_config.course_confirmed && isfinite(distance) &&
        distance >= robot_config.junction_min_m &&
        distance <= robot_config.junction_max_m &&
        held_ms >= robot_config.junction_hold_ms;
}
bool mission_finish_guard(uint32_t actions, float distance, uint32_t held_ms)
{
    return robot_config.course_confirmed &&
        actions == robot_config.finish_after_actions &&
        isfinite(distance) && distance >= robot_config.finish_min_m &&
        held_ms >= robot_config.finish_black_ms;
}
static void latch(MissionReason reason)
{
    if (!atomic_exchange_explicit(&safety_latched, true, memory_order_acq_rel))
    {
        (void)s2_stop();
        s5_scan_cancel();
    }
    atomic_store_explicit(&safety_reason, reason, memory_order_release);
    atomic_store_explicit(&moving, false, memory_order_release);
}
static bool sensors_ok(uint32_t now, const S3IrSnapshot *ir,
    const MotionFeedback *motion, const TerrainSnapshot *terrain)
{
    return ir->normalized_valid && ir->capture_kind == IR_CAPTURE_NONE &&
        now - ir->sampled_ms <= 60u && motion->valid &&
        now - motion->sampled_ms <= 60u && terrain->terrain_valid &&
        terrain->calibrated && now - terrain->timestamp_ms <= 250u &&
        terrain->state != TERRAIN_STATE_IMPACT;
}
static void safety_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    bool watchdog = false;
    for (;;)
    {
        uint32_t now;
        if (!robot_now_ms(&now)) { latch(REASON_SENSOR); return; }
        if (!atomic_load_explicit(&released, memory_order_acquire))
        {
            (void)tk_dly_tsk(10u);
            continue;
        }
        ButtonInputs buttons = drv_buttons_read();
        if (buttons.gp21_down) { latch(REASON_LOCAL_STOP); }
        MotionHealth health = {0};
        S3IrSnapshot ir = {0};
        MotionFeedback motion = {0};
        TerrainSnapshot terrain = {0};
        ObstacleSnapshot obstacle = {0};
        bool service = s2_health(&health) >= E_OK && health.sequence != 0u &&
            now - health.serviced_ms <= 60u &&
            now - atomic_load_explicit(&mission_progress, memory_order_acquire) <= 60u;
        bool copies = s3_ir_copy(&ir) && s2_feedback_copy(&motion) >= E_OK &&
            s4_get_terrain(&terrain) >= E_OK && s5_obstacle_copy(&obstacle) >= E_OK;
        if (atomic_load_explicit(&moving, memory_order_acquire))
        {
            if (!service || !copies || !sensors_ok(now, &ir, &motion, &terrain))
            {
                latch(REASON_SENSOR);
            }
            else if (health.stopped) { latch(REASON_MOTION); }
            else if (s3_barcode_faults() != 0u) { latch(REASON_BARCODE); }
            else if (!obstacle.front_valid || now - obstacle.front_ms > 250u)
            {
                latch(REASON_SENSOR);
            }
        }
        /* Watchdog supervises local work; radio/reporting never participates.
           Hardware requires x2 LOAD ticks for RP2040's documented erratum. */
#if !APP_TEST
        if (service && copies && ir.have_sample && now - ir.sampled_ms <= 60u)
        {
            out_w(0x40058004u, 500000u);
            if (!watchdog)
            {
                out_w(0x40010008u, 0x0001fffcu);
                set_w(0x40058000u, 1u << 30u);
                watchdog = true;
            }
        }
#else
        (void)watchdog;
#endif
        if (tk_dly_tsk(10u) < E_OK) { latch(REASON_SENSOR); return; }
    }
}
static void change(MissionSnapshot *s, MissionState state, MissionReason reason)
{
    if (s->state != state || s->reason != reason)
    {
        tm_printf((UB *)"[MISSION] %s reason=%s%s\n", mission_state_name(state),
            mission_reason_name(reason), APP_TEST ? " MOTOR_INHIBITED" : "");
    }
    s->state = state;
    s->reason = reason;
    s->moving = state == MISSION_FOLLOW_LINE || state == MISSION_NAVIGATION ||
        state == MISSION_BYPASS || state == MISSION_REACQUIRE;
    atomic_store_explicit(&moving, s->moving, memory_order_release);
    atomic_store_explicit(&current_mode, state, memory_order_release);
}
static bool enable_motion(MotionHealth *health)
{
    if (health->stopped)
    {
        if (!health->brake_acknowledged || s2_reset_stop() < E_OK ||
            s2_health(health) < E_OK) { return false; }
    }
    return !health->stopped;
}
static bool send(MotionCommand *command, const S3IrSnapshot *ir,
    uint32_t now, const MotionHealth *health)
{
    command->run_epoch = health->run_epoch;
    command->issued_ms = now;
    command->sampled_ms = ir->sampled_ms;
    command->expires_ms = now + 60u;
    return s2_command(command) >= E_OK;
}
static void mission_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    MissionSnapshot s = {.state = MISSION_INITIALIZING};
    Button button;
    uint32_t now = 0u;
    (void)robot_now_ms(&now);
    button_init(&button, now);
    uint32_t next = now, scan_id = 0u, scan_deadline = 0u;
    uint32_t broad_since = 0u, loss_since = 0u, encounter_ms = 0u;
    uint32_t visible_sequence = 0u, completed_id = 0u;
    unsigned visible_count = 0u, phase = 0u;
    int direction = 0, bypass_side = 1;
    bool mapped = false, have_navigation = false, scanning_requested = false;
    float run_origin = 0.0f, nav_origin = 0.0f;
    float recovery_x = 0.0f, recovery_y = 0.0f, recovery_heading = 0.0f;
    float offset = 0.0f, passage = 0.0f;
    MotionCommand command = {0};
    barcode_event_t navigation = {0};
    for (;;)
    {
        if (!robot_now_ms(&now)) { latch(REASON_SENSOR); return; }
        if (!atomic_load_explicit(&released, memory_order_acquire))
        {
            (void)tk_dly_tsk(10u);
            continue;
        }
        ButtonInputs buttons = drv_buttons_read();
        ButtonEvent event = button_update(&button, buttons.gp20_down, now);
        if (buttons.gp21_down) { button_disarm(&button); latch(REASON_LOCAL_STOP); }
        S3IrSnapshot ir = {0};
        MotionHealth health = {0};
        MotionFeedback motion = {0};
        TerrainSnapshot terrain = {0};
        ObstacleSnapshot obstacle = {0};
        barcode_status_t barcode = {0};
        bool copies = s3_ir_copy(&ir) && s2_health(&health) >= E_OK &&
            s2_feedback_copy(&motion) >= E_OK && s4_get_terrain(&terrain) >= E_OK &&
            s5_obstacle_copy(&obstacle) >= E_OK &&
            s3_barcode_copy(&barcode) == BARCODE_API_OK;
        s.configured = robot_config_mission_valid();
        float distance = (motion.left_m + motion.right_m) * 0.5f;
        if (atomic_load_explicit(&safety_latched, memory_order_acquire))
        {
            MissionReason reason = (MissionReason)atomic_load_explicit(
                &safety_reason, memory_order_acquire);
            change(&s, reason == REASON_LOCAL_STOP ? MISSION_STOPPED : MISSION_FAULT,
                reason);
            have_navigation = false;
            command = (MotionCommand){0};
            (void)s3_barcode_reset();
        }
        if (copies && !s.moving && motion.stationary && !buttons.gp21_down)
        {
            ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
            if (tk_loc_mtx(id, TMO_POL) >= E_OK)
            {
                if (mapping_pending)
                {
                    s.mapping_request_id = pending_id;
                    s.mapping_result = MAP_BUSY;
                    if (s3_barcode_reset() == BARCODE_API_OK &&
                        s3_barcode_mapping_replace(pending_mapping) == BARCODE_API_OK)
                    {
                        mapped = true;
                        for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
                        {
                            mapped = mapped && pending_mapping[i] != BARCODE_UNMAPPED;
                        }
                        s.mapping_result = MAP_APPLIED;
                    }
                    mapping_pending = false;
                }
                (void)tk_unl_mtx(id);
            }
        }
        if (s.state == MISSION_INITIALIZING)
        {
            change(&s, MISSION_CALIBRATING, REASON_CONFIGURATION);
            button_init(&button, now);
        }
        if ((s.state == MISSION_STOPPED || s.state == MISSION_FAULT ||
             s.state == MISSION_COMPLETE) && !buttons.gp21_down &&
            event == BUTTON_EVENT_PRESSED && copies && health.brake_acknowledged)
        {
            s4_reset_request(health.run_epoch, false);
            atomic_store_explicit(&safety_latched, false, memory_order_release);
            change(&s, MISSION_CALIBRATING, REASON_CONFIGURATION);
            button_init(&button, now);
            event = BUTTON_EVENT_NONE;
        }
        if (s.state == MISSION_CALIBRATING && copies && s.configured && mapped &&
            sensors_ok(now, &ir, &motion, &terrain) && motion.stationary &&
            obstacle.front_valid && now - obstacle.front_ms <= 250u)
        {
            if (barcode.state == BARCODE_DISABLED)
            {
                (void)s3_barcode_configure(&robot_config.barcode,
                    barcode_adapter_provider());
            }
            else if (barcode.fault_bits == 0u)
            {
                change(&s, MISSION_READY, REASON_NONE);
                button_init(&button, now);
                event = BUTTON_EVENT_NONE;
            }
        }
        if (s.state == MISSION_READY && event == BUTTON_EVENT_PRESSED && copies &&
            ir.guidance_valid && sensors_ok(now, &ir, &motion, &terrain) &&
            obstacle.front_valid && enable_motion(&health))
        {
            s.run_epoch = health.run_epoch;
            s.completed_actions = 0u;
            s4_reset_request(s.run_epoch, true);
            run_origin = distance;
            have_navigation = false;
            visible_count = 0u;
            broad_since = 0u;
            command = (MotionCommand){.mode = MOTION_SPEED, .action_id = ++s.action_id};
            change(&s, MISSION_FOLLOW_LINE, REASON_NONE);
        }
        if (s.moving && (!copies || !sensors_ok(now, &ir, &motion, &terrain) ||
            health.stopped || barcode.fault_bits != 0u))
        {
            latch(REASON_SENSOR);
        }
        if (s.moving && !atomic_load_explicit(&safety_latched, memory_order_acquire) &&
            obstacle.front_valid && obstacle.front_m <= robot_config.clearance_m +
                robot_config.body_length_m * 0.5f)
        {
            (void)s2_stop();
            (void)s3_barcode_reset();
            have_navigation = false;
            recovery_x = motion.x_m;
            recovery_y = motion.y_m;
            recovery_heading = motion.yaw_deg;
            scan_deadline = now + robot_config.scan_ms;
            ++scan_id;
            scanning_requested = false;
            command = (MotionCommand){0};
            change(&s, MISSION_SCAN, REASON_NONE);
        }
        if (s.state == MISSION_SCAN && copies)
        {
            if (robot_due(now, scan_deadline)) { latch(REASON_DEADLINE); }
            else if (!scanning_requested && health.brake_acknowledged &&
                motion.stationary)
            {
                scanning_requested = s5_scan_request(scan_id, scan_deadline) >= E_OK;
            }
            else if (scanning_requested && obstacle.scan_id == scan_id)
            {
                if (obstacle.scan_status == SCAN_FAILED ||
                    obstacle.scan_status == SCAN_CANCELLED) { latch(REASON_BLOCKED); }
                else if (obstacle.scan_status == SCAN_DONE && obstacle.profile_valid &&
                    obstacle.front_valid && now - obstacle.front_ms <= 250u)
                {
                    float need = robot_config.body_width_m + 2.0f * robot_config.clearance_m;
                    bypass_side = obstacle.left_clearance_m >= obstacle.right_clearance_m ? 1 : -1;
                    float available = bypass_side > 0 ? obstacle.left_clearance_m :
                        obstacle.right_clearance_m;
                    offset = obstacle.width_m * 0.5f + robot_config.body_width_m * 0.5f +
                        robot_config.clearance_m + fabsf(obstacle.obstacle_left_m);
                    passage = obstacle.obstacle_forward_m + robot_config.body_length_m +
                        robot_config.clearance_m + robot_config.obstacle_depth_bound_m;
                    float swept = hypotf(robot_config.body_width_m,
                        robot_config.body_length_m) * 0.5f + robot_config.clearance_m;
                    if (available < need || offset > robot_config.bypass_max_m ||
                        passage > robot_config.bypass_max_m || obstacle.closest_m <= swept)
                    {
                        latch(REASON_BLOCKED);
                    }
                    else if (enable_motion(&health))
                    {
                        phase = 0u;
                        completed_id = 0u;
                        command = (MotionCommand){.mode = MOTION_TURN,
                            .action_id = ++s.action_id, .target = 90.0f * bypass_side,
                            .speed_mps = robot_config.search_mps,
                            .deadline_ms = now + robot_config.action_ms};
                        change(&s, MISSION_BYPASS, REASON_NONE);
                    }
                }
            }
        }
        if (s.moving && !atomic_load_explicit(&safety_latched, memory_order_acquire))
        {
            if (s.state == MISSION_FOLLOW_LINE)
            {
                barcode_event_t candidate;
                barcode_api_result_t popped = s3_barcode_pop(&candidate);
                if (popped == BARCODE_API_OK)
                {
                    if (have_navigation || candidate.scanner_epoch != barcode.scanner_epoch ||
                        candidate.mapping_revision != barcode.mapping_revision ||
                        (uint32_t)(in_w(TIMER_TIMERAWL) - candidate.sampled_us) > 1000000u)
                    {
                        latch(REASON_AMBIGUOUS);
                    }
                    else
                    {
                        navigation = candidate;
                        have_navigation = true;
                        nav_origin = distance;
                        encounter_ms = now;
                        s.symbol = candidate.symbol;
                        s.navigation = candidate.action;
                    }
                }
                if (have_navigation && (distance - nav_origin > robot_config.junction_max_m ||
                    now - encounter_ms > robot_config.action_ms)) { latch(REASON_BARCODE); }
                if (ir.line == IR_LINE_BROAD_BLACK)
                {
                    if (broad_since == 0u) { broad_since = now; }
                    if (have_navigation && mission_junction_guard(distance - nav_origin,
                        now - broad_since))
                    {
                        phase = 0u;
                        completed_id = 0u;
                        command = (MotionCommand){.mode = MOTION_DISTANCE,
                            .action_id = ++s.action_id, .target = robot_config.junction_exit_m,
                            .speed_mps = robot_config.search_mps,
                            .deadline_ms = now + robot_config.action_ms};
                        change(&s, MISSION_NAVIGATION, REASON_NONE);
                    }
                    else if (!have_navigation && mission_finish_guard(s.completed_actions,
                        distance - run_origin, now - broad_since))
                    {
                        (void)s2_stop();
                        change(&s, MISSION_COMPLETE, REASON_NONE);
                    }
                    else if (!have_navigation &&
                        s.completed_actions != robot_config.finish_after_actions &&
                        now - broad_since >= robot_config.junction_hold_ms)
                    {
                        latch(REASON_AMBIGUOUS);
                    }
                }
                else { broad_since = 0u; }
                if (ir.line == IR_LINE_LOST)
                {
                    if (direction == 0) { latch(REASON_LINE_LOST); }
                    else
                    {
                        loss_since = now;
                        visible_count = 0u;
                        recovery_x = motion.x_m;
                        recovery_y = motion.y_m;
                        recovery_heading = motion.yaw_deg;
                        command = (MotionCommand){.mode = MOTION_SPEED,
                            .action_id = ++s.action_id};
                        change(&s, MISSION_REACQUIRE, REASON_LINE_LOST);
                    }
                }
                if (s.state == MISSION_FOLLOW_LINE)
                {
                    float speed = ir.line == IR_LINE_BROAD_BLACK ?
                        robot_config.search_mps : robot_config.cruise_mps;
                    float correction = speed * ir.steering;
                    command.left_mps = fmaxf(0.0f, fminf(speed + correction,
                        robot_config.speed_max_mps));
                    command.right_mps = fmaxf(0.0f, fminf(speed - correction,
                        robot_config.speed_max_mps));
                    if (ir.steering != 0.0f) { direction = ir.steering > 0.0f ? 1 : -1; }
                }
            }
            if ((s.state == MISSION_NAVIGATION || s.state == MISSION_BYPASS) &&
                motion.action_id == command.action_id && motion.status == MOTION_DONE &&
                completed_id != command.action_id)
            {
                completed_id = command.action_id;
                if (s2_ack(command.action_id) < E_OK) { latch(REASON_MOTION); }
                else
                {
                    ++phase;
                    unsigned end = s.state == MISSION_NAVIGATION ? 2u : 7u;
                    if (phase >= end)
                    {
                        if (s.state == MISSION_NAVIGATION) { ++s.completed_actions; }
                        have_navigation = false;
                        (void)s3_barcode_reset();
                        recovery_x = motion.x_m;
                        recovery_y = motion.y_m;
                        if (s.state == MISSION_NAVIGATION) { recovery_heading = motion.yaw_deg; }
                        visible_count = 0u;
                        loss_since = now;
                        command = (MotionCommand){.mode = MOTION_SPEED,
                            .action_id = ++s.action_id};
                        change(&s, MISSION_REACQUIRE, REASON_NONE);
                    }
                    else
                    {
                        MotionMode mode;
                        float target;
                        if (s.state == MISSION_NAVIGATION)
                        {
                            mode = navigation.action == BARCODE_STRAIGHT ?
                                MOTION_DISTANCE : MOTION_TURN;
                            target = navigation.action == BARCODE_LEFT ? 90.0f :
                                navigation.action == BARCODE_RIGHT ? -90.0f :
                                navigation.action == BARCODE_U_TURN ? 180.0f :
                                robot_config.junction_exit_m;
                        }
                        else
                        {
                            mode = phase == 1u || phase == 3u || phase == 5u ?
                                MOTION_DISTANCE : MOTION_TURN;
                            target = phase == 3u ? passage :
                                (phase == 1u || phase == 5u) ? offset :
                                phase == 6u ? 90.0f * bypass_side : -90.0f * bypass_side;
                        }
                        command = (MotionCommand){.mode = mode,
                            .action_id = ++s.action_id, .target = target,
                            .speed_mps = robot_config.search_mps,
                            .deadline_ms = now + robot_config.action_ms};
                    }
                }
            }
            if (s.state == MISSION_REACQUIRE)
            {
                float heading_error = remainderf(motion.yaw_deg - recovery_heading, 360.0f);
                bool context = hypotf(motion.x_m - recovery_x, motion.y_m - recovery_y) <=
                    robot_config.reacquire_max_m && fabsf(heading_error) <=
                    robot_config.reacquire_heading_deg;
                uint32_t limit = s.reason == REASON_LINE_LOST ? 500u : robot_config.action_ms;
                if (!context || now - loss_since >= limit) { latch(REASON_LINE_LOST); }
                if (ir.guidance_valid && ir.sequence != visible_sequence)
                {
                    ++visible_count;
                    visible_sequence = ir.sequence;
                }
                else if (!ir.guidance_valid) { visible_count = 0u; }
                if (visible_count >= 2u && context)
                {
                    (void)s3_barcode_configure(&robot_config.barcode,
                        barcode_adapter_provider());
                    command = (MotionCommand){.mode = MOTION_SPEED,
                        .action_id = ++s.action_id};
                    broad_since = 0u;
                    change(&s, MISSION_FOLLOW_LINE, REASON_NONE);
                }
                else
                {
                    command.left_mps = s.reason == REASON_LINE_LOST && direction < 0 ?
                        0.0f : robot_config.search_mps;
                    command.right_mps = s.reason == REASON_LINE_LOST && direction > 0 ?
                        0.0f : robot_config.search_mps;
                }
            }
            if (s.moving && !atomic_load_explicit(&safety_latched, memory_order_acquire) &&
                !send(&command, &ir, now, &health)) { latch(REASON_MOTION); }
        }
        s.mapping_revision = barcode.mapping_revision;
        s.safety_latched = atomic_load_explicit(&safety_latched, memory_order_acquire);
        s.sampled_ms = now;
        ++s.sequence;
        ID id = atomic_load_explicit(&snapshot_mutex, memory_order_acquire);
        if (tk_loc_mtx(id, TMO_POL) >= E_OK)
        {
            latest = s;
            (void)tk_unl_mtx(id);
        }
        atomic_store_explicit(&mission_progress, now, memory_order_release);
        next += 20u;
        if (robot_due(now, next)) { next = now + 20u; }
        if (tk_dly_tsk(next - now) < E_OK) { latch(REASON_SENSOR); return; }
    }
}
ER mission_init(void)
{
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    ID id = tk_cre_mtx(&mutex);
    if (id <= 0) { return id; }
    atomic_store_explicit(&snapshot_mutex, id, memory_order_release);
    return E_OK;
}
ER mission_start(void)
{
    ID safety = app_start_task((FP)safety_task, 12, TP_PRC2, "Safety");
    if (safety < E_OK) { return safety; }
    ID mission = app_start_task((FP)mission_task, 14, TP_PRC2, "Mission");
    return mission < E_OK ? mission : E_OK;
}
void mission_release(void)
{
    atomic_store_explicit(&released, true, memory_order_release);
}
