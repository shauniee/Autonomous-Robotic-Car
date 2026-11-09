#include "../app.h"
#include "comms.h"
#include "../mission/mission_mapping.h"
#include "../common/robot_data.h"
#include "../common/robot_time.h"
#include "../motion/motion.h"
#include "../mission/line_trace.h"
#include "../mission/mission.h"
#include "../line_barcode/line_barcode.h"
#include "../terrain/terrain.h"
#include "../obstacle/obstacle.h"

_Static_assert(MQTT_FAULT_BYTES == RD_OWNER_COUNT, "one telemetry fault byte per owner");
_Static_assert(COMMS_FRAME_MAX >= MQTT_FRAME_MAX, "bridge slots must hold the largest frame");

static comms_event_log_t event_log; /* guarded by event_mutex across cores */
static ID event_mutex;
/* Writer: comms_event callers (Mission). Letter bits 0..4 (31 none),
   action bits 5..7: one byte store. */
static _Atomic uint8_t last_barcode;
/* Writer: comms_event callers (Mission). Room on the side chosen, mm,
   0..1023 (10 of 16 bits). */
static _Atomic uint16_t last_clearance_mm;

ER comms_init(void)
{
    comms_log_init(&event_log);
    atomic_store_explicit(&last_barcode, MQTT_NO_LETTER, memory_order_release);
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    event_mutex = tk_cre_mtx(&mutex);
    return event_mutex > 0 ? E_OK : E_SYS;
}

void comms_event(mqtt_event_kind_t kind, uint8_t arg, uint16_t arg2)
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
        atomic_store_explicit(&last_barcode,
            (uint8_t)((arg & 0x1Fu) | ((arg2 & 0x7u) << 5)),
            memory_order_release);
    }
    else if (kind == MQTT_EVENT_OBSTACLE)
    {
        atomic_store_explicit(&last_clearance_mm,
            (uint16_t)(arg2 > 1023u ? 1023u : arg2), memory_order_release);
    }
    /* A lock that cannot be taken within the bound drops this event. */
    if (tk_loc_mtx(event_mutex, RD_SNAPSHOT_LOCK_MS) >= E_OK)
    {
        (void)comms_log_push(&event_log, &event);
        (void)tk_unl_mtx(event_mutex);
    }
}

void comms_barcode_clear(void)
{
    atomic_store_explicit(&last_barcode, MQTT_NO_LETTER, memory_order_release);
}

static void gather(mqtt_telemetry_t *t, uint8_t sequence)
{
    uint32_t feedback = motion_feedback(), terrain = terrain_read_word();
    obstacle_range_sample_t front_sample = {0}, side_sample = {0};
    bool have_range = obstacle_range_try_copy(&front_sample, &side_sample) >= E_OK;
    uint32_t front = front_sample.word;
    uint32_t barcode = atomic_load_explicit(&last_barcode,
        memory_order_acquire);
    LineIrSnapshot line = {0};
    ObstacleProfile profile = {0};
    bool have_line = line_ir_try_copy(&line);
    bool have_profile = obstacle_profile_try_copy(&profile) >= E_OK && profile.valid;
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
        .imu_calibration = terrain_calibrated(terrain) ?
            MQTT_IMU_CALIBRATED : MQTT_IMU_NOT_CALIBRATED,
        .front_mm = (uint16_t)echo_range(front),
        .front_valid = have_range && obstacle_range_sample_fresh(&front_sample,
            robot_time_ms(), 3u * SONAR_REPEAT_MS),
        .barcode_letter = (uint8_t)(barcode & 0x1Fu),
        .barcode_action = (uint8_t)((barcode >> 5) & 0x7u),
        .obstacle_width_mm = have_profile ? profile.width_mm : 0u,
        .obstacle_clearance_mm = atomic_load_explicit(&last_clearance_mm,
            memory_order_acquire)};
    for (unsigned i = 0u; i < MQTT_FAULT_BYTES; ++i) t->faults[i] = rd_fault_get((rd_owner_t)i);
    comms_clamp(t);
}

static void post_snapshot(mqtt_type_t type, uint8_t *sequence)
{
    mqtt_telemetry_t telemetry;
    uint8_t frame[MQTT_FRAME_MAX];
    gather(&telemetry, (*sequence)++);
    unsigned n = mqtt_encode_telemetry(frame, sizeof frame, type, &telemetry);
    if (n != 0u) (void)comms_bridge_post(frame, n);
}

/* Diagnostics are bounded even if the network continuously refills RX.
   ACKs are retained until the TX bridge accepts them. Events are popped only
   after the bridge accepts a complete frame, under the cross-core mutex. */
static void report_step(uint32_t now)
{
    static uint32_t report_due, telemetry_due;
    static uint8_t sequence;
    static uint8_t initialized;
    static uint8_t ack_pending;
    static mqtt_ack_t pending_ack;
    if (!initialized)
    {
        report_due = now;
        telemetry_due = now + COMMS_TELEMETRY_PERIOD_MS;
        initialized = 1u;
    }
    if ((int32_t)(now - report_due) < 0)
    {
        return;
    }
    report_due = now + COMMS_REPORT_PERIOD_MS;
    uint8_t out[MQTT_FRAME_MAX];
    if (!ack_pending && mission_mapping_poll(&pending_ack))
    {
        ack_pending = 1u;
    }
    if (ack_pending)
    {
        unsigned length = mqtt_encode_ack(out, sizeof out, &pending_ack);
        if (!comms_bridge_post(out, length))
        {
            return;
        }
        ack_pending = 0u;
    }
    for (unsigned budget = 0u; budget < COMMS_REQUESTS_PER_RELEASE; ++budget)
    {
        uint8_t in[COMMS_FRAME_MAX];
        unsigned length = comms_bridge_receive(in, sizeof in);
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
        else if (type == MQTT_TYPE_JUNCTION_TEST)
        {
            uint8_t seq, action;
            mqtt_decode_t decoded = mqtt_decode_junction_test(in, length, &seq, &action);
            if (decoded == MQTT_DECODE_OK && mission_direction_submit(seq, action))
            {
                continue;
            }
            pending_ack = (mqtt_ack_t){.sequence = in[1],
                .result = decoded == MQTT_DECODE_OK ? MQTT_ACK_BUSY :
                    (decoded == MQTT_DECODE_RANGE ? MQTT_ACK_INVALID : MQTT_ACK_MALFORMED)};
            ack_pending = 1u;
            break;
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
        telemetry_due = now + COMMS_TELEMETRY_PERIOD_MS;
        post_snapshot(MQTT_TYPE_TELEMETRY, &sequence);
    }
    for (unsigned i = 0u; i < COMMS_EVENTS_PER_RELEASE; ++i)
    {
        if (tk_loc_mtx(event_mutex, TMO_POL) < E_OK)
        {
            break;
        }
        bool posted = comms_log_post_oldest(&event_log, comms_bridge_post);
        (void)tk_unl_mtx(event_mutex);
        if (!posted)
        {
            break;
        }
    }
}

void comms_step(uint32_t now)
{
    /* Normal ACK/telemetry/events get first use of the bounded TX bridge. */
    report_step(now);
    /* Always sent, for the dashboard's IMU graphs. Before the wheel and line
       traces, which keep the bridge full (S3-U31). */
    {
        uint8_t frame[MQTT_LEN_TERRAIN_TRACE];
        unsigned length = terrain_trace_take(frame, sizeof frame);
        if (0u != length)
        {
            /* NOTE: best effort; a full bridge drops one graph sample. */
            (void)comms_bridge_post(frame, length);
        }
    }
#if ROBOT_TRACE_ENABLED
    for (unsigned budget = 0u; budget < 2u; ++budget)
    {
        uint8_t frame[MQTT_LEN_BARCODE_TRACE];
        unsigned length = barcode_trace_take(frame, sizeof frame);
        if (length == 0u)
        {
            break;
        }
        (void)comms_bridge_post(frame, length);
    }
    for (unsigned budget = 0u; budget < 2u; ++budget)
    {
        uint8_t frame[MQTT_LEN_WHEEL_TRACE];
        unsigned length = motion_trace_take(frame, sizeof frame);
        if (length == 0u)
        {
            break;
        }
        /* Offline/full TX drops diagnostics; never retry old samples. */
        (void)comms_bridge_post(frame, length);
    }
    for (unsigned budget = 0u; budget < 2u; ++budget)
    {
        uint8_t frame[MQTT_LEN_LINE_TRACE];
        unsigned length = line_trace_take(frame, sizeof frame);
        if (length == 0u)
        {
            break;
        }
        (void)comms_bridge_post(frame, length);
    }
#endif /* ROBOT_TRACE_ENABLED */
}
