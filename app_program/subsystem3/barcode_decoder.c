#include "barcode.h"
#include <math.h>
#include "../float_math.h"
#include <stddef.h>

/* Standard Code 39 nine-element masks: MSB is the first black bar.
   1=wide, 0=narrow. Table covers 0-9, A-Z, dash, dot, space, $, /, +, %. */
static const uint16_t code39[] = {
    0x034, 0x121, 0x061, 0x160, 0x031, 0x130, 0x070, 0x025, 0x124, 0x064,
    0x109, 0x049, 0x148, 0x019, 0x118, 0x058, 0x00d, 0x10c, 0x04c, 0x01c,
    0x103, 0x043, 0x142, 0x013, 0x112, 0x052, 0x007, 0x106, 0x046, 0x016,
    0x181, 0x0c1, 0x1c0, 0x091, 0x190, 0x0d0, 0x085, 0x184, 0x0c4,
    0x0a8, 0x0a2, 0x08a, 0x02a
};
static const char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%";

bool s3_barcode_config_valid(const S3BarcodeConfig *c)
{
    if ((c == NULL) || !c->configured ||
        ((c->left_polarity != 1) && (c->left_polarity != -1)) ||
        ((c->right_polarity != 1) && (c->right_polarity != -1)) ||
        (c->narrow_min_counts == 0u) ||
        (c->narrow_max_counts < c->narrow_min_counts) ||
        (c->narrow_max_counts >= INT32_MAX / 4u) ||
        !isfinite(c->wide_ratio_min) || !isfinite(c->wide_ratio_max) ||
        (c->wide_ratio_min <= 1.0f) ||
        (c->wide_ratio_max < c->wide_ratio_min) ||
        !isfinite(c->narrow_variation) || (c->narrow_variation < 1.0f) ||
        (c->narrow_variation >= c->wide_ratio_min) ||
        (c->quiet_counts < c->narrow_max_counts) ||
        (c->quiet_counts >= INT32_MAX) || (c->rearm_counts == 0u) ||
        (c->rearm_counts >= INT32_MAX) || (c->frame_timeout_ms == 0u) ||
        (c->frame_timeout_ms >= INT32_MAX / 1000u) ||
        (c->event_age_ms == 0u) || (c->event_age_ms >= INT32_MAX))
    {
        return false;
    }
    for (unsigned i = 0u; i < 26u; ++i)
    {
        if ((unsigned)c->mapping[i] > S3_NAV_UTURN)
        {
            return false;
        }
    }
    return true;
}

bool s3_barcode_travel(const S3BarcodeConfig *c,
    const S3BarcodeEdge *from, const S3BarcodeEdge *to, uint32_t *counts)
{
    if ((c == NULL) || (from == NULL) || (to == NULL) || (counts == NULL) ||
        !from->encoder_valid || !to->encoder_valid || to->ambiguous)
    {
        return false;
    }
    int64_t left = (int64_t)(int32_t)(to->left_count - from->left_count) *
        c->left_polarity;
    int64_t right = (int64_t)(int32_t)(to->right_count - from->right_count) *
        c->right_polarity;
    int64_t sum = left + right;
    if ((left < 0) || (right < 0) || (sum < 0) || (sum >= INT32_MAX))
    {
        return false;
    }
    *counts = (uint32_t)sum;
    return true;
}

static bool mask_for(const uint32_t *widths, const S3BarcodeConfig *c,
    uint16_t *mask, float *narrow)
{
    uint32_t sorted[9];
    for (unsigned i = 0u; i < 9u; ++i)
    {
        sorted[i] = widths[i];
        for (unsigned j = i; (j > 0u) && (sorted[j] < sorted[j - 1u]); --j)
        {
            uint32_t temporary = sorted[j];
            sorted[j] = sorted[j - 1u];
            sorted[j - 1u] = temporary;
        }
    }
    if ((sorted[0] < c->narrow_min_counts) ||
        (sorted[5] > c->narrow_max_counts) ||
        ((float)sorted[5] > (float)sorted[0] * c->narrow_variation))
    {
        return false;
    }
    float mean = 0.0f;
    for (unsigned i = 0u; i < 6u; ++i)
    {
        mean += (float)sorted[i] / 6.0f;
    }
    if (((float)sorted[6] < mean * c->wide_ratio_min) ||
        ((float)sorted[8] > mean * c->wide_ratio_max))
    {
        return false;
    }
    *mask = 0u;
    for (unsigned i = 0u; i < 9u; ++i)
    {
        *mask = (uint16_t)((*mask << 1u) |
            (widths[i] >= sorted[6] ? 1u : 0u));
    }
    *narrow = mean;
    return true;
}
static S3BarcodeResult decode_forward(const uint32_t *widths,
    const S3BarcodeConfig *c, char *symbol)
{
    uint16_t masks[3];
    float narrow[3];
    for (unsigned i = 0u; i < 3u; ++i)
    {
        if (!mask_for(&widths[i * 10u], c, &masks[i], &narrow[i]))
        {
            return S3_BC_BAD_WIDTH;
        }
    }
    if ((masks[0] != 0x094u) || (masks[2] != 0x094u))
    {
        return S3_BC_BAD_FRAME;
    }
    for (unsigned i = 0u; i < 2u; ++i)
    {
        float gap = (float)widths[9u + i * 10u];
        float minimum = robot_float_min(narrow[i], narrow[i + 1u]);
        float maximum = robot_float_max(narrow[i], narrow[i + 1u]);
        if ((gap < minimum / c->narrow_variation) ||
            (gap > maximum * c->narrow_variation))
        {
            return S3_BC_BAD_WIDTH;
        }
    }
    for (unsigned i = 0u; i < sizeof(code39) / sizeof(code39[0]); ++i)
    {
        if (masks[1] == code39[i])
        {
            *symbol = alphabet[i];
            return S3_BC_OK;
        }
    }
    return S3_BC_BAD_FRAME;
}
S3BarcodeResult s3_code39_decode(const uint32_t widths[S3_BARCODE_WIDTHS],
    const S3BarcodeConfig *c, char *symbol)
{
    if ((widths == NULL) || (symbol == NULL) || !s3_barcode_config_valid(c))
    {
        return S3_BC_UNCONFIGURED;
    }
    char decoded;
    S3BarcodeResult result = decode_forward(widths, c, &decoded);
    if (result == S3_BC_OK)
    {
        *symbol = decoded;
        return result;
    }
    uint32_t reverse[S3_BARCODE_WIDTHS];
    for (unsigned i = 0u; i < S3_BARCODE_WIDTHS; ++i)
    {
        reverse[i] = widths[S3_BARCODE_WIDTHS - i - 1u];
    }
    if (decode_forward(reverse, c, &decoded) == S3_BC_OK)
    {
        if (!c->allow_reverse)
        {
            return S3_BC_REVERSE;
        }
        *symbol = decoded;
        return S3_BC_OK;
    }
    return result;
}
void s3_barcode_decoder_init(S3BarcodeDecoder *d,
    const S3BarcodeConfig *c, uint32_t generation)
{
    *d = (S3BarcodeDecoder){0};
    if (c != NULL)
    {
        d->config = *c;
    }
    d->status.configured = s3_barcode_config_valid(c);
    d->status.generation = generation;
    d->status.state = d->status.configured ? S3_BC_QUIET : S3_BC_DISABLED;
    d->status.result = d->status.configured ? S3_BC_OK : S3_BC_UNCONFIGURED;
}
void s3_barcode_decoder_context(S3BarcodeDecoder *d, uint32_t generation,
    bool clear_fault)
{
    uint32_t accepted = d->status.accepted;
    uint32_t rejected = d->status.rejected;
    uint32_t discarded = d->status.discarded + d->count;
    bool fault = d->status.state == S3_BC_FAULT;
    S3BarcodeResult result = d->status.result;
    S3BarcodeConfig configuration = d->config;
    s3_barcode_decoder_init(d, &configuration, generation);
    d->status.accepted = accepted;
    d->status.rejected = rejected;
    d->status.discarded = discarded;
    if (fault && !clear_fault)
    {
        d->status.state = S3_BC_FAULT;
        d->status.result = result;
    }
}

void s3_barcode_decoder_fault(S3BarcodeDecoder *d, S3BarcodeResult result)
{
    d->status.result = result;
    d->status.state = S3_BC_FAULT;
    ++d->status.rejected;
    d->count = 0u;
    d->status.queued = 0u;
}
static void reject(S3BarcodeDecoder *d, S3BarcodeResult result)
{
    ++d->status.rejected;
    d->status.result = result;
    d->status.state = S3_BC_QUIET;
    d->width_count = 0u;
    d->have_white = false;
}
void s3_barcode_decoder_edge(S3BarcodeDecoder *d, const S3BarcodeEdge *e)
{
    if ((d == NULL) || (e == NULL) || !d->status.configured ||
        (d->status.state == S3_BC_FAULT))
    {
        return;
    }
    if (e->ambiguous)
    {
        s3_barcode_decoder_fault(d, S3_BC_CAPTURE_OVERFLOW);
        return;
    }
    if ((e->generation != d->status.generation) ||
        !e->encoder_valid)
    {
        reject(d, S3_BC_ENCODER);
        d->have_previous = false;
        return;
    }
    bool black = e->high == d->config.black_high;
    if (d->have_previous && (e->high == d->previous.high))
    {
        reject(d, S3_BC_BAD_FRAME);
    }
    if (d->status.state == S3_BC_REARM)
    {
        d->previous = *e;
        d->have_previous = true;
        return;
    }
    uint32_t distance = 0u;
    bool measured = d->have_previous &&
        s3_barcode_travel(&d->config, &d->previous, e, &distance);
    if (d->status.state == S3_BC_CAPTURE)
    {
        if ((e->context != d->entry.context) || !measured ||
            (distance == 0u))
        {
            reject(d, S3_BC_CONTEXT);
        }
        else if (d->width_count == S3_BARCODE_WIDTHS)
        {
            reject(d, S3_BC_BAD_FRAME);
        }
        else
        {
            d->widths[d->width_count++] = distance;
            if ((d->width_count == S3_BARCODE_WIDTHS) && !black)
            {
                d->exit = *e;
            }
        }
    }
    else if (black && d->have_white && measured &&
        (distance >= d->config.quiet_counts))
    {
        d->entry = *e;
        d->frame_started_us = e->captured_us;
        d->width_count = 0u;
        d->status.state = S3_BC_CAPTURE;
    }
    if (!black && (d->status.state == S3_BC_QUIET))
    {
        d->have_white = true;
    }
    d->previous = *e;
    d->have_previous = true;
}
void s3_barcode_decoder_poll(S3BarcodeDecoder *d,
    const S3BarcodeEdge *o, uint32_t now)
{
    ++d->status.sequence;
    d->status.serviced_ms = now;
    d->status.encoder_valid = o->encoder_valid;
    d->status.context = o->context;
    if ((d->status.state == S3_BC_FAULT) || !d->status.configured)
    {
        return;
    }
    if (!d->have_previous)
    {
        /* Startup on white establishes only an observed white interval. */
        if ((o->high != d->config.black_high) && o->encoder_valid)
        {
            d->previous = *o;
            d->have_previous = true;
            d->have_white = true;
        }
        return;
    }
    uint32_t quiet = 0u;
    bool white = o->high != d->config.black_high;
    bool measured = s3_barcode_travel(&d->config, &d->previous, o, &quiet);
    if (d->status.state == S3_BC_REARM)
    {
        uint32_t rearm = 0u;
        if (white && measured && (quiet >= d->config.quiet_counts) &&
            s3_barcode_travel(&d->config, &d->exit, o, &rearm) &&
            (rearm >= d->config.rearm_counts))
        {
            d->status.state = S3_BC_QUIET;
            d->have_white = true;
        }
        return;
    }
    if (d->status.state != S3_BC_CAPTURE)
    {
        return;
    }
    if ((uint32_t)(o->captured_us - d->frame_started_us) >=
        d->config.frame_timeout_ms * 1000u)
    {
        reject(d, S3_BC_PARTIAL);
        return;
    }
    if ((d->width_count != S3_BARCODE_WIDTHS) || !white || !measured ||
        (quiet < d->config.quiet_counts))
    {
        return;
    }
    char symbol = '\0';
    S3BarcodeResult result = s3_code39_decode(d->widths, &d->config, &symbol);
    d->status.symbol = symbol;
    d->exit = d->previous;
    d->status.state = S3_BC_REARM;
    d->status.result = result;
    if (result != S3_BC_OK)
    {
        ++d->status.rejected;
        return;
    }
    S3NavAction action = ((symbol >= 'A') && (symbol <= 'Z')) ?
        d->config.mapping[(unsigned)(symbol - 'A')] : S3_NAV_NONE;
    if (action == S3_NAV_NONE)
    {
        d->status.result = S3_BC_UNMAPPED;
        ++d->status.rejected;
        return;
    }
    if ((d->count == S3_NAV_CAPACITY) || (d->next_id == UINT32_MAX))
    {
        s3_barcode_decoder_fault(d, S3_BC_QUEUE_OVERFLOW);
        return;
    }
    unsigned slot = (d->head + d->count) % S3_NAV_CAPACITY;
    d->events[slot] = (S3NavEvent){.id = ++d->next_id,
        .acquired_ms = now - (uint32_t)(o->captured_us - d->exit.captured_us) /
            1000u,
        .captured_us = d->exit.captured_us, .context = d->entry.context,
        .generation = d->status.generation, .symbol = symbol, .action = action,
        .left_count = d->exit.left_count, .right_count = d->exit.right_count};
    ++d->count;
    ++d->status.accepted;
    d->status.queued = d->count;
}
bool s3_barcode_decoder_take(S3BarcodeDecoder *d, S3NavEvent *event)
{
    if ((d == NULL) || (event == NULL) || (d->count == 0u) ||
        (d->status.state == S3_BC_FAULT))
    {
        return false;
    }
    *event = d->events[d->head];
    d->head = (d->head + 1u) % S3_NAV_CAPACITY;
    --d->count;
    d->status.queued = d->count;
    return true;
}
const char *s3_barcode_state_name(S3BarcodeState state)
{
    switch (state)
    {
        case S3_BC_DISABLED: return "disabled";
        case S3_BC_QUIET: return "quiet";
        case S3_BC_CAPTURE: return "capture";
        case S3_BC_REARM: return "rearm";
        case S3_BC_FAULT: return "fault";
        default: return "fault";
    }
}
const char *s3_barcode_result_name(S3BarcodeResult result)
{
    static const char *const names[] = {"ok", "unconfigured", "partial",
        "bad_width", "bad_frame", "reverse", "unmapped", "encoder",
        "capture_overflow", "queue_overflow", "context", "stale_event"};
    return (unsigned)result < sizeof(names) / sizeof(names[0]) ?
        names[result] : "invalid";
}
