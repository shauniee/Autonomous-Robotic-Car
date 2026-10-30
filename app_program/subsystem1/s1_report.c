#include "s1_report.h"
#include <stddef.h>

/* Private accessors: no caller manipulates packed bits directly. */
enum
{
    S1_KIND_MASK = 0x07u,   /* 3-bit event kind */
    S1_ARG_MASK = 0x7Fu,    /* 7-bit event argument */
    S1_HEAD_MASK = 0x0Fu,  /* 4-bit ring head */
    S1_COUNT_MASK = 0x1Fu, /* 5-bit ring count */
    S1_ARG_SHIFT = 3u,
    S1_COUNT_SHIFT = 4u
};

static bool s1_event_pack(s1_packed_event_t *dst, const mqtt_event_t *src)
{
    if (dst == 0 || src == 0 || src->kind == 0u ||
        src->kind >= MQTT_EVENT_COUNT || src->arg > S1_ARG_MASK)
        return false;

    dst->kind_arg = (uint16_t)(((uint16_t)src->arg << S1_ARG_SHIFT) | src->kind);
    dst->time_ms = src->time_ms;
    dst->left_count = src->left_count;
    dst->right_count = src->right_count;
    dst->arg2 = src->arg2;
    return true;
}

static void s1_event_unpack(mqtt_event_t *dst, const s1_packed_event_t *src)
{
    *dst = (mqtt_event_t){
        .kind = (uint8_t)(src->kind_arg & S1_KIND_MASK),
        .arg = (uint8_t)((src->kind_arg >> S1_ARG_SHIFT) & S1_ARG_MASK),
        .time_ms = src->time_ms,
        .left_count = src->left_count,
        .right_count = src->right_count,
        .arg2 = src->arg2
    };
}

static unsigned s1_log_head(const s1_event_log_t *log)
{
    return (unsigned)(log->indices & S1_HEAD_MASK);
}

unsigned s1_log_count(const s1_event_log_t *log)
{
    return log == 0 ? 0u : (unsigned)((log->indices >> S1_COUNT_SHIFT) & S1_COUNT_MASK);
}

static void s1_log_set_indices(s1_event_log_t *log, unsigned head, unsigned count)
{
    log->indices = (uint16_t)((head & S1_HEAD_MASK) |
                              ((count & S1_COUNT_MASK) << S1_COUNT_SHIFT));
}

void s1_log_init(s1_event_log_t *log)
{
    if (log != 0) *log = (s1_event_log_t){0};
}

bool s1_log_push(s1_event_log_t *log, const mqtt_event_t *event)
{
    if (log == 0) return false;
    s1_packed_event_t packed;
    if (!s1_event_pack(&packed, event)) return false; /* no truncation */

    unsigned head = s1_log_head(log);
    unsigned count = s1_log_count(log);
    if (count == S1_EVENT_CAPACITY)
    {
        if (log->dropped != 0xFFFFFFFFu) ++log->dropped;
        /* head is also the oldest slot when full: overwrite it. */
    }
    else
    {
        ++count;
    }
    log->events[head] = packed;
    head = (head + 1u) % S1_EVENT_CAPACITY;
    s1_log_set_indices(log, head, count);
    return true;
}

bool s1_log_pop(s1_event_log_t *log, mqtt_event_t *event)
{
    if (log == 0 || event == 0) return false;
    unsigned count = s1_log_count(log);
    if (count == 0u) return false;
    unsigned oldest = (s1_log_head(log) + S1_EVENT_CAPACITY - count) % S1_EVENT_CAPACITY;
    s1_event_unpack(event, &log->events[oldest]);
    s1_log_set_indices(log, s1_log_head(log), count - 1u);
    return true;
}

bool s1_log_post_oldest(s1_event_log_t *log, s1_post_fn post)
{
    unsigned count = s1_log_count(log);
    if (count == 0u || post == NULL)
    {
        return false;
    }
    unsigned oldest = (s1_log_head(log) + S1_EVENT_CAPACITY - count) % S1_EVENT_CAPACITY;
    mqtt_event_t event;
    s1_event_unpack(&event, &log->events[oldest]);
    uint8_t frame[MQTT_FRAME_MAX];
    unsigned length = mqtt_encode_event(frame, sizeof frame, &event);
    if (length == 0u || !post(frame, length))
    {
        return false;
    }
    s1_log_set_indices(log, s1_log_head(log), count - 1u);
    return true;
}

static uint8_t clamp8(unsigned v, unsigned max) { return (uint8_t)(v > max ? max : v); }

void s1_clamp(mqtt_telemetry_t *t)
{
    t->mission_state = clamp8(t->mission_state, 15u);
    t->motion_status = clamp8(t->motion_status, 3u);
    t->motion_action = clamp8(t->motion_action, 63u);
    t->line_status = clamp8(t->line_status, 3u);
    t->hump_max = clamp8(t->hump_max, 127u);
    t->front_mm = (uint16_t)(t->front_mm > 8191u ? 8191u : t->front_mm);
    t->barcode_letter = t->barcode_letter > 25u ? MQTT_NO_LETTER : t->barcode_letter;
    t->barcode_action = clamp8(t->barcode_action, 7u);
}
