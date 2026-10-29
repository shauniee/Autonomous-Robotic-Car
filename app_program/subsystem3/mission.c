#include "mission.h"
#include <stdatomic.h>
#include "subsystem3.h"
#include "safety.h"
#include "../subsystem2/subsystem2.h"
#include "../subsystem5/subsystem5.h"
#include "../subsystem1/subsystem1.h"
#include "../subsystem2/drv_encoder.h"
#include "../common/buttons.h"
#include "../common/robot_time.h"

static _Atomic uint8_t published_state;	/* writer: Mission task; 8 states, 3 bits */

MissionState mission_state(void)
{
    return (MissionState)atomic_load_explicit(&published_state, memory_order_acquire);
}

ER mission_set_mapping(const uint8_t letters[4])
{
    MissionState state = mission_state();
    if (state == MISSION_FOLLOW || state == MISSION_JUNCTION_TURN || state == MISSION_BYPASS ||
        state == MISSION_CROSSING)
    {
        return E_OBJ;
    }
    return barcode_set_mapping(letters) ? E_OK : E_OBJ;
}

static const char *const state_names[MISSION_STATE_COUNT] = {
    "CALIBRATE", "READY", "FOLLOW", "JUNCTION_TURN", "BYPASS", "COMPLETE", "STOP", "CROSSING"};

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
        safety_step();
        ButtonInputs buttons = drv_buttons_read();
        if (buttons.gp21_down || mission.state == MISSION_STOP)
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
        uint32_t front = s5_front_word(), side = s5_side_word();
        ObstacleProfile profile = {0};
        bool have_profile = s5_profile_copy(&profile) >= E_OK;
        barcode_event_t barcode = {0};
        MissionInputs in = {
            .now_ms = now,
            .start_pressed = event == BUTTON_EVENT_PRESSED,
            .mapped = barcode_mapped(),
            .start_blocked = rd_fault_start_blocked(),
            .stop_down = buttons.gp21_down,
            .safety_latched = safety_latched_causes() != 0u,
            .motion_stopped = !healthy || motion_health_stopped(&health),
            .inputs_ok = copied && healthy,
            .calibration_valid = snapshot.calibration_valid,
            .guidance_valid = snapshot.guidance_valid,
            .usable_line = snapshot.normalized_valid && snapshot.capture_kind == IR_CAPTURE_NONE,
            .line = snapshot.line,
            .line_error = snapshot.error,
            .line_sampled_ms = snapshot.sampled_ms,
            .travel_counts = enc_mean(drv_encoder_count(ENCODER_LEFT), drv_encoder_count(ENCODER_RIGHT)),
            .motion_status = rd_motion_feedback_status(feedback),
            .motion_action = (uint8_t)rd_motion_feedback_action(feedback),
            .stationary = robot_time_elapsed(now, still_since) >= MISSION_STATIONARY_MS,
            .front_valid = echo_valid(front),
            .front_age_ms = (now - echo_time16(front)) & 0xFFFFu,
            .front_mm = echo_range(front),
            .side_valid = echo_valid(side) && ((now - echo_time16(side)) & 0xFFFFu) <= 3u * SONAR_REPEAT_MS,
            .side_mm = echo_range(side),
            .scan_state = s5_scan_state_unpack(s5_scan_word() & rd_mask(S5_SCAN_STATE_BITS)),
            .profile_valid = have_profile && profile.valid,
            .left_clearance_mm = profile.left_clearance_mm,
            .right_clearance_mm = profile.right_clearance_mm,
            .have_barcode = barcode_pop(&barcode),
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
            if (mission.state == MISSION_STOP)
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
