#include "subsystem1.h"
#include "s1_bridge.h"
#include "s1_report.h"
#include "../common/robot_data.h"
#include "../common/robot_time.h"
#include "../subsystem2/subsystem2.h"
#include "../subsystem2/drv_encoder.h"
#include "../mission/mission.h"
#include "../subsystem3/barcode.h"
#include "../subsystem3/subsystem3.h"
#include "../subsystem4/subsystem4.h"
#include "../subsystem5/subsystem5.h"

_Static_assert(MQTT_FAULT_BYTES == RD_OWNER_COUNT, "one telemetry fault byte per owner");
_Static_assert(S1_FRAME_MAX >= MQTT_FRAME_MAX, "bridge slots must hold the largest frame");

static s1_event_log_t event_log;	/* guarded by DI: producers and Reporting share core 1 */
static rd_word_t last_barcode;		/* writer: s1_event callers (Mission); letter | action << 5 */
static rd_word_t last_depth_mm;		/* writer: s1_event callers (Mission); latest obstacle depth */

ER subsystem1_init(void)
{
    s1_log_init(&event_log);
    rd_word_publish(&last_barcode, MQTT_NO_LETTER);
    return E_OK; /* Optional reporting must not inhibit local control. */
}

void s1_event(mqtt_event_kind_t kind, uint8_t arg, uint16_t arg2)
{
    mqtt_event_t event = {
        .kind = (uint8_t)kind,
        .arg = arg,
        .time_ms = (uint16_t)(robot_time_ms() & 0xFFFFu),
        .left_count = (uint16_t)drv_encoder_count(ENCODER_LEFT),
        .right_count = (uint16_t)drv_encoder_count(ENCODER_RIGHT),
        .arg2 = arg2};
    if (kind == MQTT_EVENT_BARCODE)
    {
        rd_word_publish(&last_barcode, (uint32_t)(arg & 0x1Fu) | ((uint32_t)(arg2 & 0x7u) << 5));
    }
    else if (kind == MQTT_EVENT_OBSTACLE)
    {
        rd_word_publish(&last_depth_mm, arg2);
    }
    UW state;
    DI(state);
    s1_log_push(&event_log, &event);
    EI(state);
}

static void gather(mqtt_telemetry_t *t, uint8_t sequence)
{
    uint32_t feedback = s2_feedback(), terrain = s4_terrain_word(), front = s5_front_word();
    uint32_t barcode = rd_word_read(&last_barcode);
    S3IrSnapshot line = {0};
    ObstacleProfile profile = {0};
    bool have_line = s3_ir_copy(&line);
    bool have_profile = s5_profile_copy(&profile) >= E_OK && profile.valid;
    *t = (mqtt_telemetry_t){
        .sequence = sequence,
        .mission_state = (uint8_t)mission_state(),
        .motion_status = (uint8_t)rd_motion_feedback_status(feedback),
        .motion_action = (uint8_t)rd_motion_feedback_action(feedback),
        .line_status = (uint8_t)(have_line ? line.line : IR_LINE_UNKNOWN),
        .mapped = barcode_mapped(),
        .left_count = (uint16_t)drv_encoder_count(ENCODER_LEFT),
        .right_count = (uint16_t)drv_encoder_count(ENCODER_RIGHT),
        .hump_count = (uint8_t)terrain_hump_count(terrain),
        .hump_max = (uint8_t)terrain_max_code(terrain),
        /* The maximum is valid once any hump completed, whatever the latest climb. */
        .hump_valid = terrain_hump_count(terrain) != 0u,
        .front_mm = (uint16_t)echo_range(front),
        .front_valid = echo_valid(front),
        .barcode_letter = (uint8_t)(barcode & 0x1Fu),
        .barcode_action = (uint8_t)((barcode >> 5) & 0x7u),
        .obstacle_width_mm = have_profile ? profile.width_mm : 0u,
        .obstacle_depth_mm = (uint16_t)rd_word_read(&last_depth_mm)};
    for (unsigned i = 0u; i < MQTT_FAULT_BYTES; ++i) t->faults[i] = rd_fault_get((rd_owner_t)i);
    s1_clamp(t);
}

static bool install_mapping(const uint8_t letters[4])
{
    return mission_set_mapping(letters) >= E_OK;
}

static bool robot_stopped(void)
{
    MissionState state = mission_state();
    return state == MISSION_READY || state == MISSION_STOPPED;
}

static void post_snapshot(mqtt_type_t type, uint8_t *sequence)
{
    mqtt_telemetry_t telemetry;
    uint8_t frame[MQTT_FRAME_MAX];
    gather(&telemetry, (*sequence)++);
    unsigned n = mqtt_encode_telemetry(frame, sizeof frame, type, &telemetry);
    if (n != 0u) (void)s1_bridge_post(frame, n);
}

static void reporting_task(INT stacd, void *exinf)
{
    (void)stacd; (void)exinf;
    uint32_t release = robot_time_ms();
    uint8_t sequence = 0u;
    unsigned elapsed_ms = 0u;
    for (;;)
    {
        /* Commands: a mapping is acknowledged only after Mission's install attempt. */
        uint8_t in[S1_FRAME_MAX], out[MQTT_FRAME_MAX];
        for (unsigned n; (n = s1_bridge_receive(in, sizeof in)) != 0u;)
        {
            mqtt_ack_t ack = {0};
            bool send_ack;
            s1_command_t command = s1_handle_command(in, n, robot_stopped(), install_mapping, &ack, &send_ack);
            if (command == S1_COMMAND_SNAPSHOT) post_snapshot(MQTT_TYPE_SNAPSHOT_REPLY, &sequence);
            if (send_ack)
            {
                unsigned len = mqtt_encode_ack(out, sizeof out, &ack);
                if (len != 0u) (void)s1_bridge_post(out, len);
            }
        }

        elapsed_ms += S1_REPORT_PERIOD_MS;
        if (elapsed_ms >= S1_TELEMETRY_PERIOD_MS)
        {
            elapsed_ms = 0u;
            post_snapshot(MQTT_TYPE_TELEMETRY, &sequence);
        }

        for (unsigned i = 0u; i < S1_EVENTS_PER_RELEASE; ++i)
        {
            mqtt_event_t event;
            UW state;
            DI(state);
            bool have = s1_log_pop(&event_log, &event);
            EI(state);
            if (!have) break;
            unsigned len = mqtt_encode_event(out, sizeof out, &event);
            if (len != 0u && !s1_bridge_post(out, len)) break;	/* outgoing ring full: later */
        }
        tk_dly_tsk(robot_time_next_delay(&release, S1_REPORT_PERIOD_MS, robot_time_ms()));
    }
}

ER subsystem1_start(void)
{
    ID tid = app_start_task((FP)reporting_task, S1_PRI_REPORTING, TP_PRC2, "Reporting");
    return tid < E_OK ? tid : E_OK;
}
