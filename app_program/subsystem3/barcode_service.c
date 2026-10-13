#include "barcode_service.h"
#include <stddef.h>

void barcode_service_init(barcode_service_t *s)
{
    if (s != NULL)
    {
        *s = (barcode_service_t){0};
    }
}

void barcode_service_reset(barcode_service_t *s, barcode_result_t reason)
{
    if (s == NULL)
    {
        return;
    }
    ++s->status.scanner_epoch;
    s->status.state = BARCODE_DISABLED;
    s->status.result = reason;
    s->status.fault_bits = 0u;
    s->status.queued_events = 0u;
    s->status.have_decode = false;
    s->status.latest = (barcode_event_t){0};
    s->event_head = 0u;
    s->have_position = false;
    s->pending_frame = false;
    s->count = 0u;
    s->run_um = 0u;
    s->frame_travel_um = 0u;
    s->travel_direction = 0;
}

bool barcode_service_configure(barcode_service_t *s,
    const barcode_config_t *config)
{
    if ((s == NULL) || !barcode_config_valid(config) ||
        (s->status.state != BARCODE_DISABLED))
    {
        return false;
    }
    s->config = *config;
    s->status.state = BARCODE_WAIT_QUIET;
    s->status.result = BARCODE_NO_RESULT;
    return true;
}

bool barcode_service_mapping(barcode_service_t *s,
    const barcode_action_t actions[BARCODE_ALPHABET])
{
    if ((s == NULL) || (actions == NULL) ||
        ((s->status.state != BARCODE_DISABLED) &&
         (s->status.state != BARCODE_WAIT_QUIET)))
    {
        return false;
    }
    for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
    {
        if ((unsigned)actions[i] > (unsigned)BARCODE_U_TURN)
        {
            return false;
        }
    }
    for (unsigned i = 0u; i < BARCODE_ALPHABET; ++i)
    {
        s->mapping[i] = actions[i];
    }
    ++s->status.mapping_revision;
    return true;
}

static void reject(barcode_service_t *s, barcode_result_t reason)
{
    s->status.result = reason;
    s->status.have_decode = false;
    s->status.latest = (barcode_event_t){0};
    ++s->status.rejected_reads;
    s->status.state = BARCODE_WAIT_QUIET;
    s->pending_frame = false;
    s->have_position = false; /* Establish a new white baseline. */
    s->count = 0u;
    s->run_um = 0u;
    s->travel_direction = 0;
}

void barcode_service_reject(barcode_service_t *s, barcode_result_t reason)
{
    if ((s != NULL) && (s->status.state != BARCODE_DISABLED) &&
        (s->status.state != BARCODE_FAULT))
    {
        reject(s, reason);
    }
}

void barcode_service_fault(barcode_service_t *s, barcode_result_t reason)
{
    if (s != NULL)
    {
        s->status.state = BARCODE_FAULT;
        s->status.result = reason;
        s->status.fault_bits |= reason == BARCODE_EVENT_OVERFLOW ?
            BARCODE_FAULT_QUEUE : BARCODE_FAULT_CAPTURE;
        s->pending_frame = false;
    }
}

static int64_t count_delta(uint32_t current, uint32_t previous)
{
    uint32_t delta = current - previous;
    return delta < 0x80000000u ? (int64_t)delta :
        (int64_t)delta - INT64_C(4294967296);
}

static uint64_t magnitude(int64_t value)
{
    return (uint64_t)(value < 0 ? -value : value);
}

static bool travel(const barcode_service_t *s, const barcode_encoder_t *e,
    uint32_t *distance, int *direction)
{
    int64_t left = count_delta(e->left_count, s->previous.left_count) *
        s->config.left_um_per_count;
    int64_t right = count_delta(e->right_count, s->previous.right_count) *
        s->config.right_um_per_count;
    if ((magnitude(left) > s->config.max_step_um) ||
        (magnitude(right) > s->config.max_step_um))
    {
        return false;
    }
    /* Subtraction/sum are safe after bounding each displacement to uint32. */
    if ((magnitude(left - right) > s->config.max_wheel_difference_um) ||
        ((left < 0) && (right > 0)) || ((left > 0) && (right < 0)))
    {
        return false;
    }
    int64_t centre = (left + right) / 2;
    *distance = (uint32_t)magnitude(centre);
    *direction = centre > 0 ? 1 : (centre < 0 ? -1 : 0);
    return true;
}

static void finish_frame(barcode_service_t *s)
{
    char symbol;
    barcode_orientation_t orientation;
    barcode_result_t result = barcode_decode(s->widths, &s->config,
        &symbol, &orientation);
    s->pending_frame = false;
    if (result != BARCODE_DECODED)
    {
        reject(s, result);
        return;
    }
    barcode_event_t event =
    {
        .encounter_id = ++s->encounter_id,
        .sampled_us = s->last_edge_us,
        .mapping_revision = s->status.mapping_revision,
        .scanner_epoch = s->status.scanner_epoch,
        .symbol = symbol,
        .action = s->mapping[(unsigned)(symbol - 'A')],
        .orientation = orientation
    };
    s->status.latest = event;
    s->status.have_decode = true;
    s->status.result = BARCODE_DECODED;
    if (event.action == BARCODE_UNMAPPED)
    {
        return; /* Decoded observation, never an invented command. */
    }
    if (s->status.queued_events == BARCODE_EVENT_CAPACITY)
    {
        barcode_service_fault(s, BARCODE_EVENT_OVERFLOW);
        return;
    }
    unsigned tail = (s->event_head + s->status.queued_events) %
        BARCODE_EVENT_CAPACITY;
    s->events[tail] = event;
    ++s->status.queued_events;
}

void barcode_service_check_time(barcode_service_t *s, uint32_t now_us)
{
    if ((s != NULL) && ((s->status.state == BARCODE_CAPTURE) ||
        s->pending_frame) &&
        (((uint32_t)(now_us - s->frame_started_us) >=
            s->config.max_frame_us) ||
         ((uint32_t)(now_us - s->last_edge_us) >=
            s->config.max_edge_gap_us)))
    {
        reject(s, BARCODE_TIMEOUT);
    }
}

void barcode_service_observe(barcode_service_t *s, bool level,
    const barcode_encoder_t *e, uint32_t sampled_us, bool edge)
{
    if ((s == NULL) || (s->status.state == BARCODE_DISABLED) ||
        (s->status.state == BARCODE_FAULT))
    {
        return;
    }
    barcode_service_check_time(s, sampled_us);
    bool black = level == s->config.black_high;
    if ((e == NULL) || !e->valid ||
        ((uint32_t)(sampled_us - e->sampled_us) >
            s->config.max_encoder_age_us))
    {
        reject(s, BARCODE_BAD_ENCODER);
        return;
    }
    if (!s->have_position)
    {
        s->previous = *e;
        s->last_observed_us = sampled_us;
        s->black = black;
        s->have_position = true;
        s->run_um = 0u;
        return; /* A first black sample cannot establish a quiet zone. */
    }
    uint32_t distance;
    int direction;
    if ((e->epoch != s->previous.epoch) ||
        ((uint32_t)(sampled_us - s->last_observed_us) >= INT32_MAX) ||
        (edge && (sampled_us == s->last_observed_us)) ||
        ((e->sampled_us == s->previous.sampled_us) &&
         ((e->left_count != s->previous.left_count) ||
          (e->right_count != s->previous.right_count))) ||
        ((uint32_t)(e->sampled_us - s->previous.sampled_us) >=
            INT32_MAX) || !travel(s, e, &distance, &direction))
    {
        reject(s, BARCODE_BAD_ENCODER);
        return;
    }
    s->previous = *e;
    s->last_observed_us = sampled_us;
    bool in_frame = (s->status.state == BARCODE_CAPTURE) || s->pending_frame;
    if ((direction != 0) && (s->travel_direction != 0) &&
        (direction != s->travel_direction))
    {
        if (in_frame)
        {
            reject(s, BARCODE_REVERSED_TRAVEL);
            return;
        }
        s->run_um = 0u; /* Reverse outside capture needs a new quiet zone. */
    }
    if (direction != 0)
    {
        s->travel_direction = direction;
    }
    if (UINT32_MAX - s->run_um < distance)
    {
        reject(s, BARCODE_BAD_ENCODER);
        return;
    }
    s->run_um += distance;
    if (in_frame)
    {
        int64_t left = count_delta(e->left_count,
            s->frame_origin.left_count) * s->config.left_um_per_count;
        int64_t right = count_delta(e->right_count,
            s->frame_origin.right_count) * s->config.right_um_per_count;
        /* Bound operands before subtraction, even on corrupt/reset counts. */
        if ((magnitude(left) > s->config.max_frame_travel_um) ||
            (magnitude(right) > s->config.max_frame_travel_um) ||
            (magnitude(left - right) >
                s->config.max_wheel_difference_um))
        {
            reject(s, BARCODE_BAD_ENCODER);
            return;
        }
        if ((distance > s->config.max_frame_travel_um) ||
            (s->frame_travel_um >
                s->config.max_frame_travel_um - distance))
        {
            reject(s, BARCODE_BAD_WIDTH);
            return;
        }
        s->frame_travel_um += distance;
    }
    /* Polls may not conceal transitions lost by the GPIO capture path. */
    if ((!edge && (black != s->black)) || (edge && (black == s->black)))
    {
        reject(s, BARCODE_BAD_EDGE);
        return;
    }
    if ((s->status.state == BARCODE_WAIT_REARM) && !s->black)
    {
        if (s->pending_frame && (s->run_um >= s->config.quiet_zone_um))
        {
            finish_frame(s);
            if (!s->have_position || (s->status.state == BARCODE_FAULT))
            {
                return;
            }
        }
        if (!s->pending_frame && (s->run_um >= s->config.rearm_um))
        {
            s->status.state = BARCODE_WAIT_QUIET;
        }
        else if (edge && s->pending_frame)
        {
            reject(s, BARCODE_SHORT_QUIET);
            return;
        }
    }
    if (!edge)
    {
        return;
    }
    if ((s->status.state == BARCODE_WAIT_QUIET) && black)
    {
        if (s->run_um < s->config.quiet_zone_um)
        {
            reject(s, BARCODE_SHORT_QUIET);
            return;
        }
        s->status.state = BARCODE_CAPTURE;
        s->count = 0u;
        s->frame_travel_um = 0u;
        s->frame_started_us = sampled_us;
        s->frame_origin = *e;
        s->last_edge_us = sampled_us;
        s->status.have_decode = false;
    }
    else if (s->status.state == BARCODE_CAPTURE)
    {
        if ((s->run_um == 0u) || (s->count >= BARCODE_ELEMENTS))
        {
            reject(s, BARCODE_BAD_WIDTH);
            return;
        }
        s->widths[s->count++] = s->run_um;
        s->last_edge_us = sampled_us;
        if (s->count == BARCODE_ELEMENTS)
        {
            s->status.state = BARCODE_WAIT_REARM;
            s->pending_frame = true;
        }
    }
    s->black = black;
    s->run_um = 0u;
}

bool barcode_service_pop(barcode_service_t *s, barcode_event_t *event)
{
    if ((s == NULL) || (event == NULL) ||
        (s->status.queued_events == 0u) ||
        (s->status.state == BARCODE_FAULT))
    {
        return false;
    }
    *event = s->events[s->event_head];
    s->event_head = (uint8_t)((s->event_head + 1u) % BARCODE_EVENT_CAPACITY);
    --s->status.queued_events;
    return true;
}
