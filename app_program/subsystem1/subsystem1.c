#include "subsystem1.h"
#include "s1_bridge.h"
#include "s1_report.h"
#include "../mission/mission_mapping.h"
#include "../common/robot_data.h"
#include "../common/robot_time.h"
#include "../common/robot_snapshot.h"
#include "../subsystem2/subsystem2.h"
#include "../subsystem2/drv_encoder.h"
#include "../mission/mission.h"
#include "../subsystem3/barcode.h"
#include "../subsystem3/subsystem3.h"
#include "../subsystem4/subsystem4.h"
#include "../subsystem5/subsystem5.h"

_Static_assert(MQTT_FAULT_BYTES == RD_OWNER_COUNT, "one telemetry fault byte per owner");
_Static_assert(S1_FRAME_MAX >= MQTT_FRAME_MAX, "bridge slots must hold the largest frame");

static s1_event_log_t event_log; /* guarded by event_mutex across cores */
static ID event_mutex;
/* Each core is its own counter's sole writer. Atomic load/store permits
   diagnostic reads without a cross-core RMW or a second contended lock. */
static rd_word_t lock_misses[2];
static rd_word_t last_barcode;		/* writer: s1_event callers (Mission); letter | action << 5 */
static rd_word_t last_depth_mm;		/* writer: s1_event callers (Mission); latest obstacle depth */

ER subsystem1_init(void)
{
    s1_log_init(&event_log);
    rd_word_publish(&last_barcode, MQTT_NO_LETTER);
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    event_mutex = tk_cre_mtx(&mutex);
    return event_mutex > 0 ? E_OK : E_SYS;
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
    if (tk_loc_mtx(event_mutex, RD_SNAPSHOT_LOCK_MS) >= E_OK)
    {
        (void)s1_log_push(&event_log, &event);
        (void)tk_unl_mtx(event_mutex);
    }
    else
    {
        /* tk_get_prc is 1-based; callers are pinned to cores 0 and 1. */
        unsigned core = (unsigned)tk_get_prc() - 1u;
        if (core < 2u)
        {
            uint32_t misses = rd_word_read(&lock_misses[core]);
            if (misses != UINT32_MAX)
            {
                rd_word_publish(&lock_misses[core], misses + 1u);
            }
        }
    }
}

static void gather(mqtt_telemetry_t *t, uint8_t sequence)
{
    uint32_t feedback = s2_feedback(), terrain = s4_terrain_word();
    s5_range_sample_t front_sample = {0}, side_sample = {0};
    bool have_range = s5_range_try_copy(&front_sample, &side_sample) >= E_OK;
    uint32_t front = front_sample.word;
    uint32_t barcode = rd_word_read(&last_barcode);
    S3IrSnapshot line = {0};
    ObstacleProfile profile = {0};
    bool have_line = s3_ir_try_copy(&line);
    bool have_profile = s5_profile_try_copy(&profile) >= E_OK && profile.valid;
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
        .front_valid = have_range && s5_range_sample_fresh(&front_sample,
            robot_time_ms(), 3u * SONAR_REPEAT_MS),
        .barcode_letter = (uint8_t)(barcode & 0x1Fu),
        .barcode_action = (uint8_t)((barcode >> 5) & 0x7u),
        .obstacle_width_mm = have_profile ? profile.width_mm : 0u,
        .obstacle_depth_mm = (uint16_t)rd_word_read(&last_depth_mm)};
    for (unsigned i = 0u; i < MQTT_FAULT_BYTES; ++i) t->faults[i] = rd_fault_get((rd_owner_t)i);
    s1_clamp(t);
}

static void post_snapshot(mqtt_type_t type, uint8_t *sequence)
{
    mqtt_telemetry_t telemetry;
    uint8_t frame[MQTT_FRAME_MAX];
    gather(&telemetry, (*sequence)++);
    unsigned n = mqtt_encode_telemetry(frame, sizeof frame, type, &telemetry);
    if (n != 0u) (void)s1_bridge_post(frame, n);
}

/* Diagnostics are bounded even if the network continuously refills RX.
   ACKs are retained until the TX bridge accepts them. Events are popped only
   after the bridge accepts a complete frame, under the cross-core mutex. */
void s1_step(uint32_t now)
{
    static uint32_t report_due, telemetry_due;
    static uint8_t sequence;
    static uint8_t initialized;
    static uint8_t ack_pending;
    static mqtt_ack_t pending_ack;
    if (!initialized)
    {
        report_due = now;
        telemetry_due = now + S1_TELEMETRY_PERIOD_MS;
        initialized = 1u;
    }
    if ((int32_t)(now - report_due) < 0)
    {
        return;
    }
    report_due = now + S1_REPORT_PERIOD_MS;
    uint8_t out[MQTT_FRAME_MAX];
    if (!ack_pending && mission_mapping_poll(&pending_ack))
    {
        ack_pending = 1u;
    }
    if (ack_pending)
    {
        unsigned length = mqtt_encode_ack(out, sizeof out, &pending_ack);
        if (!s1_bridge_post(out, length))
        {
            return;
        }
        ack_pending = 0u;
    }
    for (unsigned budget = 0u; budget < S1_REQUESTS_PER_RELEASE; ++budget)
    {
        uint8_t in[S1_FRAME_MAX];
        unsigned length = s1_bridge_receive(in, sizeof in);
        if (length == 0u)
        {
            break;
        }
        mqtt_type_t type;
        if (mqtt_frame_type(in, length, &type) != MQTT_DECODE_OK)
        {
            continue;
        }
        if (type == MQTT_TYPE_SNAPSHOT_REQUEST)
        {
            post_snapshot(MQTT_TYPE_SNAPSHOT_REPLY, &sequence);
        }
        else if (type == MQTT_TYPE_MAPPING)
        {
            mqtt_mapping_t mapping;
            mqtt_decode_t decoded = mqtt_decode_mapping(in, length, &mapping);
            if (decoded == MQTT_DECODE_OK && mission_mapping_submit(&mapping))
            {
                continue; /* Only Mission's installed result may ACK OK. */
            }
            pending_ack = (mqtt_ack_t){.sequence = in[1],
                .result = decoded == MQTT_DECODE_OK ? MQTT_ACK_BUSY :
                    (decoded == MQTT_DECODE_RANGE ? MQTT_ACK_INVALID : MQTT_ACK_MALFORMED)};
            ack_pending = 1u;
            break;
        }
    }
    if ((int32_t)(now - telemetry_due) >= 0)
    {
        telemetry_due = now + S1_TELEMETRY_PERIOD_MS;
        post_snapshot(MQTT_TYPE_TELEMETRY, &sequence);
    }
    for (unsigned i = 0u; i < S1_EVENTS_PER_RELEASE; ++i)
    {
        if (tk_loc_mtx(event_mutex, TMO_POL) < E_OK)
        {
            break;
        }
        bool posted = s1_log_post_oldest(&event_log, s1_bridge_post);
        (void)tk_unl_mtx(event_mutex);
        if (!posted)
        {
            break;
        }
    }
}
