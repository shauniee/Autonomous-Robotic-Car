#include "mission.h"
#include "mission_mapping.h"
#include "line_trace.h"
#include "../terrain/terrain.h"
#include <stdatomic.h>
#include "../line_barcode/line_barcode.h"
#include "safety.h"
#include "../motion/motion.h"
#include "../obstacle/obstacle.h"
#include "../comms/comms.h"
#include "../common/buttons.h"
#include "../common/robot_time.h"
#include "../common/robot_watchdog.h"

static _Atomic uint8_t published_state;	/* writer: Mission task; 7 states, 3 logical bits */

MissionState mission_state(void)
{
    return (MissionState)atomic_load_explicit(&published_state, memory_order_acquire);
}

static const char *const state_names[MISSION_STATE_COUNT] = {
    "READY", "FOLLOW_LINE", "NAVIGATE", "SCAN", "BYPASS", "REACQUIRE", "STOPPED"};

static void mission_task(INT stacd, void *exinf)
{
    (void)stacd; (void)exinf;
    uint32_t now = robot_time_ms();
    uint32_t release = now;
    Mission mission;
    mission_init(&mission, now);
    Button start_button;
    button_init(&start_button, now);
    MissionState reported = MISSION_STATE_COUNT;
    uint32_t last_left = drv_encoder_count(ENCODER_LEFT), last_right = drv_encoder_count(ENCODER_RIGHT);
    uint32_t still_since = now;
    bool side_watching = false;
#if ROBOT_TRACE_ENABLED
    uint32_t traced_ms = 0u;
#endif

    for (;;)
    {
        now = robot_time_ms();
        line_barcode_step(now);
        mqtt_mapping_t mapping;
        uint8_t direction;
        if (mission_mapping_take(&mapping, &direction))
        {
            /* The state decision and installation have one owner. An I/O
               observation of READY can never race a local start here. */
            mqtt_ack_result_t result = MQTT_ACK_BUSY;
            if (direction != 0u)
            {
                result = mission_queue_direction(&mission, direction) ?
                    MQTT_ACK_OK : MQTT_ACK_BUSY;
            }
            else if (mission.state == MISSION_READY || mission.state == MISSION_STOPPED)
            {
                result = barcode_set_mapping(mapping.letters) ?
                    MQTT_ACK_OK : MQTT_ACK_INVALID;
                if (result == MQTT_ACK_OK)
                {
                    mission.held = BARCODE_UNMAPPED;
                }
            }
            mission_mapping_reply(result);
        }
        obstacle_step(now);
        bool line_required = mission.state == MISSION_FOLLOW_LINE ||
            mission.state == MISSION_REACQUIRE ||
            mission.state == MISSION_NAVIGATE ||
            (mission.state == MISSION_BYPASS &&
             mission.bypass.route == BYPASS_ROUTE_RETURN);
        safety_step(line_required);
        ButtonInputs buttons = drv_buttons_read();
        if (buttons.gp21_down || mission.state == MISSION_STOPPED)
        {
            button_disarm(&start_button);
        }
        ButtonEvent event = button_update(&start_button, buttons.gp20_down, now);
        if (event != BUTTON_EVENT_NONE)
        {
            tm_printf((UB *)"[MISSION] GP20 %s state=%s GP21_down=%d\n",
                event == BUTTON_EVENT_PRESSED ? "pressed" :
                (event == BUTTON_EVENT_RELEASED ? "released" :
                    "ignored: release GP20 then press again"),
                state_names[mission.state], (int)buttons.gp21_down);
        }
#if APP_TEST
        /* TEST never leaves its boot stop, so GP20 scans in place for
           S5-U10. */
        if (event == BUTTON_EVENT_PRESSED && mission.state == MISSION_READY)
        {
            static uint8_t bench_scan_id;
            bench_scan_id = (uint8_t)((bench_scan_id + 1u) & 15u);
            obstacle_scan_request(bench_scan_id, false);
        }
#endif

        LineIrSnapshot snapshot = {0};
        MotionHealth health = {0};
        bool copied = line_ir_copy(&snapshot);
        bool healthy = motion_health(&health) >= E_OK;
        now = robot_time_ms();
        ir_snapshot_refresh(&snapshot, now);

        uint32_t feedback = motion_feedback();
        uint32_t left_now = drv_encoder_count(ENCODER_LEFT), right_now = drv_encoder_count(ENCODER_RIGHT);
        if (left_now != last_left || right_now != last_right)
        {
            last_left = left_now;
            last_right = right_now;
            still_since = now;
        }
        obstacle_range_sample_t front_sample = {0}, side_sample = {0};
        bool range_copied = obstacle_range_samples(&front_sample, &side_sample) >= E_OK;
        uint32_t front = front_sample.word, side = side_sample.word;
        obstacle_profile_snapshot_t scan_profile = {0};
        bool have_profile = obstacle_profile_snapshot(&scan_profile) >= E_OK &&
            robot_time_elapsed(now, scan_profile.sampled_ms) <= OBSTACLE_SCAN_DEADLINE_MS &&
            scan_profile.evidence.left_edge_valid && scan_profile.evidence.right_edge_valid;
        ObstacleProfile profile = scan_profile.profile;
        uint32_t scan = obstacle_scan_word();
        barcode_event_t barcode = {0};
        bool have_barcode = barcode_pop(&barcode);
        MissionInputs in = {
            .now_ms = now,
            .start_pressed = event == BUTTON_EVENT_PRESSED,
            .mapped = barcode_mapped(),
            .start_blocked = rd_fault_start_blocked() || !comms_bridge_ready() ||
                !terrain_startup_complete() || !copied || !snapshot.sample_fresh ||
                !healthy || health.sequence == 0u ||
                robot_time_elapsed(now, health.serviced_ms) > SAFETY_STALE_MS,
            .stop_down = buttons.gp21_down,
            .safety_latched = safety_latched_causes() != 0u,
            .motion_stopped = !healthy || motion_health_stopped(&health),
            .inputs_ok = healthy && (!line_required ||
                (copied && snapshot.sample_fresh && snapshot.normalized_valid)),
            .guidance_valid = snapshot.guidance_valid,
            .usable_line = snapshot.normalized_valid,
            .line = snapshot.line,
            .line_error = snapshot.error,
            .left_black = snapshot.left_fraction != 0u,
            .right_black = snapshot.right_fraction != 0u,
            .line_sampled_ms = snapshot.sampled_ms,
            .wheel_count = {left_now, right_now},
            .motion_status = rd_motion_feedback_status(feedback),
            .motion_action = (uint8_t)rd_motion_feedback_action(feedback),
            .stationary = robot_time_elapsed(now, still_since) >= MISSION_STATIONARY_MS,
            .front_valid = range_copied && front_sample.sequence != 0u && echo_valid(front),
            .front_age_ms = robot_time_elapsed(now, front_sample.sampled_ms),
            .front_sampled_ms = front_sample.sampled_ms,
            .front_sequence = front_sample.sequence,
            .front_wheel_count = {front_sample.wheel_count[0],
                front_sample.wheel_count[1]},
            .front_mm = echo_range(front),
            .side_valid = range_copied && side_sample.sequence != 0u && echo_valid(side) &&
                robot_time_elapsed(now, side_sample.sampled_ms) <= 3u * SONAR_REPEAT_MS,
            .side_mm = echo_range(side),
            .side_sampled_ms = side_sample.sampled_ms,
            .side_sequence = side_sample.sequence,
            .scan_state = obstacle_scan_state_unpack(scan & rd_mask(OBSTACLE_SCAN_STATE_BITS)),
            .scan_id = obstacle_scan_id(scan),
            .profile_valid = have_profile && profile.valid,
            .profile_closest_mm = profile.closest_mm,
            .profile_width_mm = profile.width_mm,
            .profile_planning_mm = scan_profile.evidence.planning_width_mm,
            .profile_forward_mm = profile.obstacle_forward_mm,
            .profile_left_mm = profile.obstacle_left_mm,
            .left_clearance_mm = profile.left_clearance_mm,
            .right_clearance_mm = profile.right_clearance_mm,
            .have_barcode = have_barcode,
            .barcode_level = gpio_get_val(BARCODE_GPIO) != 0,
            .barcode_edges = (uint8_t)(drv_barcode_edge_count() & 31u),
            .barcode_action = barcode_event_action(&barcode),
            .barcode_letter = barcode_event_letter(&barcode)};

        MissionOutputs out;
        bool was_recovering = mission.state == MISSION_REACQUIRE;
#if ROBOT_TRACE_ENABLED
        bool was_moving = mission.state != MISSION_READY && mission.state != MISSION_STOPPED;
#endif
        mission_step(&mission, &in, &out);
        /* One record per fresh acquisition while moving, including the stop. */
#if ROBOT_TRACE_ENABLED
        if (was_moving && in.line_sampled_ms != traced_ms)
        {
            mqtt_line_trace_t trace = mission_line_trace(&mission, &in, &out);
            (void)line_trace_publish(&trace);	/* full ring drops diagnostics only */
            traced_ms = in.line_sampled_ms;
        }
#endif /* ROBOT_TRACE_ENABLED */

        if (out.stop_now && motion_stop() < E_OK)
        {
            tm_printf((UB *)"[MISSION] stop request failed\n");
        }
        if (out.nav_rejected != 0u)
        {
            tm_printf((UB *)"[MISSION] navigation inhibited reason=%u "
                "(1=measurements 2=geometry 3=entry)\n", (unsigned)out.nav_rejected);
        }
        /* Every decoded barcode needs a route event, including a barcode
           consumed during a navigation transition or a manual test command.
           Mission normally publishes its own event in FOLLOW; avoid duplicates.
           Logging never grants motor/navigation authority. */
        if (mission_barcode_event_needed(&in, &out))
        {
            comms_event(MQTT_EVENT_BARCODE, in.barcode_letter,
                (uint16_t)in.barcode_action);
        }
        if (out.event_kind != 0u)
        {
            comms_event((mqtt_event_kind_t)out.event_kind, out.event_arg, out.event_arg2);
        }
        if (out.start_refused)
        {
            tm_printf((UB *)"[MISSION] start refused: mapped=%d "
                "start_blocked=%d motion_stopped=%d safety_latched=%d "
                "guidance=%d line=%u ir_fresh=%d network_ready=%d "
                "imu_startup_done=%d motion_fresh=%d TEST=%d\n",
                (int)in.mapped, (int)in.start_blocked,
                (int)in.motion_stopped, (int)in.safety_latched,
                (int)in.guidance_valid, (unsigned)in.line,
                (int)(copied && snapshot.sample_fresh),
                (int)comms_bridge_ready(), (int)terrain_startup_complete(),
                (int)(healthy && health.sequence != 0u &&
                    robot_time_elapsed(now, health.serviced_ms) <=
                        SAFETY_STALE_MS), APP_TEST);
            tm_printf((UB *)"[MISSION] start faults: motion=%u line=%u "
                "terrain=%u\n", (unsigned)rd_fault_get(RD_OWNER_MOTION),
                (unsigned)rd_fault_get(RD_OWNER_LINE),
                (unsigned)rd_fault_get(RD_OWNER_TERRAIN));
        }
        if (out.line_lost)
        {
            rd_fault_set(RD_OWNER_MISSION, RD_FAULT_LINE_LOST);
        }
        if (out.start_scanner)
        {
            rd_fault_clear(RD_OWNER_MISSION, RD_FAULT_LINE_LOST);
            if (!barcode_start())
            {
                tm_printf((UB *)"[MISSION] barcode scanner unavailable\n");
            }
        }
        if (out.stop_scanner)
        {
            barcode_stop();
        }
        if (out.scan_request)
        {
            obstacle_scan_request(out.scan_id, out.scan_keep_narrower);
        }
        if (out.scan_cancel)
        {
            obstacle_scan_cancel();
        }
        if (out.side_watch_on != side_watching || out.side_watch_on)
        {
            side_watching = out.side_watch_on;
            obstacle_side_watch(out.side_watch_on, out.side_watch_deg);
        }
        if (motion_command(&out.command) < E_OK)
        {
            tm_printf((UB *)"[MISSION] command write failed; stopping\n");
            (void)motion_stop();
        }
        else
        {
            /* Mission progress is acknowledged only after command handoff. */
            robot_watchdog_note_mission(robot_time_ms());
        }
        if (was_recovering && out.line_lost)
        {
            /* Report only after STOP has been requested and commanded.
               These are Mission's original failure inputs, not a later sample. */
            int32_t left = enc_travel_signed(in.wheel_count[0],
                mission.recovery.origin[0]);
            int32_t right = enc_travel_signed(in.wheel_count[1],
                mission.recovery.origin[1]);
            float heading = enc_heading_deg(left, right);
            float left_abs = left < 0 ? -(float)left : (float)left;
            float right_abs = right < 0 ? -(float)right : (float)right;
            float excursion = (left_abs + right_abs) * 0.5f /
                ENC_COUNTS_PER_MM;
            tm_printf((UB *)"[RECOVERY] failed: elapsed_ms=%u "
                "left_counts=%d right_counts=%d heading_cdeg=%d "
                "excursion_mm=%d right=%u usable_ir=%d ir_age_ms=%u "
                "front_valid=%d front_age_ms=%u front_mm=%u\n",
                (unsigned)robot_time_elapsed(in.now_ms,
                    mission.recovery.started_ms),
                (int)left, (int)right, (int)(heading * 100.0f),
                (int)excursion, (unsigned)mission.recovery.right,
                (int)in.usable_line,
                (unsigned)robot_time_elapsed(in.now_ms, in.line_sampled_ms),
                (int)in.front_valid, (unsigned)in.front_age_ms,
                (unsigned)in.front_mm);
        }
        atomic_store_explicit(&published_state, (uint8_t)mission.state, memory_order_release);
        if (mission.state != reported)
        {
            reported = mission.state;
            tm_printf((UB *)"[MISSION] state=%s%s\n", state_names[mission.state], APP_TEST ? " MOTOR_INHIBITED" : "");
            if (mission.state == MISSION_STOPPED)
            {
                tm_printf((UB *)"[MISSION] stop: gp21=%d latched=%u "
                    "inputs_ok=%d motion_stopped=%d motion_status=%u\n",
                    (int)in.stop_down, (unsigned)safety_latched_causes(),
                    (int)in.inputs_ok, (int)in.motion_stopped,
                    (unsigned)in.motion_status);
            }
        }
        tk_dly_tsk(robot_time_next_delay(&release, FOLLOW_PERIOD_MS, robot_time_ms()));
    }
}

ER mission_start(void)
{
    ID id = app_start_task((FP)mission_task, FOLLOW_PRIORITY, TP_PRC2, "Mission");
    return id < E_OK ? id : E_OK;
}
