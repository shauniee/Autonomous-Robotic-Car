#include "mission.h"
#include "mission_mapping.h"
#include "../subsystem4/subsystem4.h"
#include <stdatomic.h>
#include "../subsystem3/subsystem3.h"
#include "safety.h"
#include "../subsystem2/subsystem2.h"
#include "../subsystem5/subsystem5.h"
#include "../subsystem1/subsystem1.h"
#include "../subsystem1/s1_bridge.h"
#include "../subsystem2/drv_encoder.h"
#include "../common/buttons.h"
#include "../common/robot_time.h"

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

    for (;;)
    {
        now = robot_time_ms();
        s3_step(now, s4_startup_complete());
        mqtt_mapping_t mapping;
        if (mission_mapping_take(&mapping))
        {
            /* The state decision and installation have one owner. An I/O
               observation of READY can never race a local start here. */
            mqtt_ack_result_t result = MQTT_ACK_BUSY;
            if (mission.state == MISSION_READY || mission.state == MISSION_STOPPED)
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
        s5_step(now);
        bool line_required = mission.state == MISSION_FOLLOW_LINE ||
            mission.state == MISSION_REACQUIRE ||
            (mission.state == MISSION_NAVIGATE &&
             (mission.phase == JUNCTION_PHASE_CROSS ||
              mission.phase == JUNCTION_PHASE_CONFIRM)) ||
            (mission.state == MISSION_BYPASS &&
             mission.bypass_step == BYPASS_REACQUIRE);
        safety_step(line_required);
        ButtonInputs buttons = drv_buttons_read();
        if (buttons.gp21_down || mission.state == MISSION_STOPPED)
        {
            button_disarm(&start_button);
        }
        ButtonEvent event = button_update(&start_button, buttons.gp20_down, now);

        S3IrSnapshot snapshot = {0};
        MotionHealth health = {0};
        bool copied = s3_ir_copy(&snapshot);
        bool healthy = s2_health(&health) >= E_OK;
        now = robot_time_ms();
        ir_snapshot_refresh(&snapshot, now);

        uint32_t feedback = s2_feedback();
        uint32_t left_now = drv_encoder_count(ENCODER_LEFT), right_now = drv_encoder_count(ENCODER_RIGHT);
        if (left_now != last_left || right_now != last_right)
        {
            last_left = left_now;
            last_right = right_now;
            still_since = now;
        }
        s5_range_sample_t front_sample = {0}, side_sample = {0};
        bool range_copied = s5_range_samples(&front_sample, &side_sample) >= E_OK;
        uint32_t front = front_sample.word, side = side_sample.word;
        s5_profile_snapshot_t scan_profile = {0};
        bool have_profile = s5_profile_snapshot(&scan_profile) >= E_OK &&
            robot_time_elapsed(now, scan_profile.sampled_ms) <= S5_SCAN_DEADLINE_MS &&
            scan_profile.evidence.left_edge_valid && scan_profile.evidence.right_edge_valid;
        ObstacleProfile profile = scan_profile.profile;
        S4Snapshot terrain = {0};
        bool have_terrain = s4_copy(&terrain) &&
            terrain_evidence_fresh(&terrain, now, SAFETY_STALE_MS) &&
            terrain_calibrated(terrain.terrain);
        uint32_t terrain_state = terrain_state_code(terrain.terrain);
        barcode_event_t barcode = {0};
        bool have_barcode = barcode_pop(&barcode);
        MissionInputs in = {
            .now_ms = now,
            .start_pressed = event == BUTTON_EVENT_PRESSED,
            .mapped = barcode_mapped(),
            .start_blocked = rd_fault_start_blocked() || !s1_bridge_ready() ||
                !s4_startup_complete() || !copied || !snapshot.sample_fresh ||
                !healthy || health.sequence == 0u ||
                robot_time_elapsed(now, health.serviced_ms) > SAFETY_STALE_MS,
            .stop_down = buttons.gp21_down,
            .safety_latched = safety_latched_causes() != 0u,
            .motion_stopped = !healthy || motion_health_stopped(&health),
            .inputs_ok = healthy && (!line_required ||
                (copied && snapshot.sample_fresh && snapshot.normalized_valid)),
            .calibration_valid = snapshot.calibration_valid,
            .guidance_valid = snapshot.guidance_valid,
            .usable_line = snapshot.normalized_valid && snapshot.capture_kind == IR_CAPTURE_NONE,
            .line = snapshot.line,
            .line_error = snapshot.error,
            .line_sampled_ms = snapshot.sampled_ms,
            .wheel_count = {left_now, right_now},
            .travel_counts = enc_mean(drv_encoder_count(ENCODER_LEFT), drv_encoder_count(ENCODER_RIGHT)),
            .motion_status = rd_motion_feedback_status(feedback),
            .motion_action = (uint8_t)rd_motion_feedback_action(feedback),
            .hump_active = have_terrain &&
                (terrain_state == TERRAIN_STATE_CLIMBING ||
                 terrain_state == TERRAIN_STATE_CRESTING ||
                 terrain_state == TERRAIN_STATE_DESCENDING),
            .stationary = robot_time_elapsed(now, still_since) >= MISSION_STATIONARY_MS,
            .front_valid = range_copied && front_sample.sequence != 0u && echo_valid(front),
            .front_age_ms = robot_time_elapsed(now, front_sample.sampled_ms),
            .front_mm = echo_range(front),
            .side_valid = range_copied && side_sample.sequence != 0u && echo_valid(side) &&
                robot_time_elapsed(now, side_sample.sampled_ms) <= 3u * SONAR_REPEAT_MS,
            .side_mm = echo_range(side),
            .scan_state = s5_scan_state_unpack(s5_scan_word() & rd_mask(S5_SCAN_STATE_BITS)),
            .profile_valid = have_profile && profile.valid,
            .left_clearance_mm = profile.left_clearance_mm,
            .right_clearance_mm = profile.right_clearance_mm,
            .have_barcode = have_barcode,
            .barcode_action = barcode_event_action(&barcode),
            .barcode_letter = barcode_event_letter(&barcode)};

        MissionOutputs out;
        mission_step(&mission, &in, &out);

        if (out.stop_now && s2_stop() < E_OK)
        {
            tm_printf((UB *)"[MISSION] stop request failed\n");
        }
        if (out.event_kind != 0u)
        {
            s1_event((mqtt_event_kind_t)out.event_kind, out.event_arg, out.event_arg2);
        }
        if (out.start_refused)
        {
            tm_printf((UB *)"[MISSION] start refused: %s (reported in telemetry)\n", in.mapped ? "fault set" : "no barcode mapping");
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
            s5_scan_request(0u);
        }
        if (out.scan_cancel)
        {
            s5_scan_cancel();
        }
        if (out.side_watch_on != side_watching || out.side_watch_on)
        {
            side_watching = out.side_watch_on;
            s5_side_watch(out.side_watch_on, out.side_watch_deg);
        }
        if (s2_command(&out.command) < E_OK)
        {
            tm_printf((UB *)"[MISSION] command write failed; stopping\n");
            (void)s2_stop();
        }
        atomic_store_explicit(&published_state, (uint8_t)mission.state, memory_order_release);
        if (mission.state != reported)
        {
            reported = mission.state;
            tm_printf((UB *)"[MISSION] state=%s%s\n", state_names[mission.state], APP_TEST ? " MOTOR_INHIBITED" : "");
            if (mission.state == MISSION_STOPPED)
            {
                tm_printf((UB *)"[MISSION] stop: gp21=%d latched=%d inputs_ok=%d\n",
                          (int)in.stop_down, (int)in.safety_latched, (int)in.inputs_ok);
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
