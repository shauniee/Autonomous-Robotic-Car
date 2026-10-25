#include "mission.h"
#include <stdatomic.h>
#include "subsystem3.h"
#include "safety.h"
#include "../subsystem2/subsystem2.h"
#include "../subsystem2/drv_encoder.h"
#include "../common/button.h"
#include "../common/drv_buttons.h"
#include "../common/robot_time.h"

static _Atomic uint32_t published_state;	/* writer: Mission task */

MissionState mission_state(void)
{
    return (MissionState)atomic_load_explicit(&published_state, memory_order_acquire);
}

ER mission_set_mapping(const uint8_t letters[4])
{
    MissionState state = mission_state();
    if (state == MISSION_FOLLOW || state == MISSION_JUNCTION_TURN)
    {
        return E_OBJ;
    }
    return barcode_set_mapping(letters) ? E_OK : E_OBJ;
}

/* Wrap-safe mean of the two wheel counts (a plain average breaks at the wrap). */
static uint32_t mean_travel_count(void)
{
    uint32_t left = drv_encoder_count(ENCODER_LEFT);
    uint32_t right = drv_encoder_count(ENCODER_RIGHT);
    return enc_advance(left, enc_travel_signed(right, left) / 2);
}

static const char *const state_names[MISSION_STATE_COUNT] = {
    "CALIBRATE", "READY", "FOLLOW", "JUNCTION_TURN", "COMPLETE", "STOP"};

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

    for (;;)
    {
        now = robot_time_ms();
        ButtonInputs buttons = drv_buttons_read();
        if (buttons.gp21_down)
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
        barcode_event_t barcode = {0};
        MissionInputs in = {
            .now_ms = now,
            .start_pressed = event == BUTTON_EVENT_PRESSED,
            .stop_down = buttons.gp21_down,
            .safety_latched = safety_latched_causes() != 0u,
            .motion_stopped = !healthy || health.stopped,
            .inputs_ok = copied && healthy,
            .calibration_valid = snapshot.calibration_valid,
            .guidance_valid = snapshot.guidance_valid,
            .usable_line = snapshot.normalized_valid && snapshot.capture_kind == IR_CAPTURE_NONE,
            .line = snapshot.line,
            .steering = snapshot.steering,
            .line_sampled_ms = snapshot.sampled_ms,
            .travel_counts = mean_travel_count(),
            .motion_status = rd_motion_feedback_status(feedback),
            .motion_action = (uint8_t)rd_motion_feedback_action(feedback),
            .have_barcode = barcode_pop(&barcode),
            .barcode_action = barcode.action};

        MissionOutputs out;
        mission_step(&mission, &in, &out);

        if (out.stop_now && s2_stop() < E_OK)
        {
            tm_printf((UB *)"[MISSION] stop request failed\n");
        }
        if (out.request_reset)
        {
            safety_request_reset();
        }
        if (out.start_scanner && !barcode_start())
        {
            tm_printf((UB *)"[MISSION] barcode scanner not started (unmapped or unavailable)\n");
        }
        if (out.stop_scanner)
        {
            barcode_stop();
        }
        if (s2_command(&out.command) < E_OK)
        {
            tm_printf((UB *)"[MISSION] command write failed; stopping\n");
            (void)s2_stop();
        }
        atomic_store_explicit(&published_state, (uint32_t)mission.state, memory_order_release);
        if (mission.state != reported)
        {
            reported = mission.state;
            tm_printf((UB *)"[MISSION] state=%s%s\n", state_names[mission.state], APP_TEST ? " MOTOR_INHIBITED" : "");
        }
        tk_dly_tsk(robot_time_next_delay(&release, FOLLOW_PERIOD_MS, robot_time_ms()));
    }
}

ER mission_start(void)
{
    ID id = app_start_task((FP)mission_task, FOLLOW_PRIORITY, TP_PRC2, "Mission");
    return id < E_OK ? id : E_OK;
}
