#include "telemetry.h"
#include <stddef.h>

typedef struct
{
    char *data;
    unsigned length, capacity;
    bool ok;
} Json;
static void character(Json *j, char c)
{
    if (j->length + 1 >= j->capacity)
    {
        j->ok = false;
        return;
    }
    j->data[j->length++] = c;
}
static void text(Json *j, const char *s)
{
    while (*s && j->ok)
        character(j, *s++);
}
static void number(Json *j, uint32_t n)
{
    char digits[10];
    unsigned count = 0;
    do
    {
        digits[count++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    while (count)
        character(j, digits[--count]);
}
static void boolean(Json *j, bool b)
{
    text(j, b ? "true" : "false");
}
static void decimal(Json *j, int16_t value)
{
    int32_t n = value;
    if (n < 0)
    {
        character(j, '-');
        n = -n;
    }
    number(j, (uint32_t)n / 1000u);
    character(j, '.');
    character(j, (char)('0' + (n / 100) % 10));
    character(j, (char)('0' + (n / 10) % 10));
    character(j, (char)('0' + n % 10));
}
static void quoted(Json *j, const char *s, unsigned capacity)
{
    static const char hex[] = "0123456789abcdef";
    character(j, '"');
    for (unsigned i = 0; i < capacity; ++i)
    {
        unsigned char c = (unsigned char)s[i];
        if (!c)
        {
            character(j, '"');
            return;
        }
        if (c == '"' || c == '\\')
        {
            character(j, '\\');
            character(j, (char)c);
        }
        else if (c < 32)
        {
            text(j, "\\u00");
            character(j, hex[c / 16]);
            character(j, hex[c % 16]);
        }
        else
            character(j, (char)c);
    }
    j->ok =
        false; /* Unterminated public input is rejected, never read beyond. */
}
static void reference(Json *j, bool valid, uint16_t left, uint16_t right)
{
    if (!valid)
    {
        text(j, "null");
        return;
    }
    text(j, "{\"left\":");
    number(j, left);
    text(j, ",\"right\":");
    number(j, right);
    character(j, '}');
}

static void signed_number(Json *j, int32_t n)
{
    if (n < 0)
    {
        character(j, '-');
        number(j, 0u - (uint32_t)n);
    }
    else
    {
        number(j, (uint32_t)n);
    }
}
static void integer_pair(Json *j, bool valid, int32_t a, int32_t b)
{
    if (!valid)
    {
        text(j, "null");
        return;
    }
    character(j, '[');
    signed_number(j, a);
    character(j, ',');
    signed_number(j, b);
    character(j, ']');
}
static void integer_triple(Json *j, bool valid, const int32_t values[3])
{
    if (!valid)
    {
        text(j, "null");
        return;
    }
    character(j, '[');
    for (unsigned i = 0u; i < 3u; ++i)
    {
        if (i != 0u)
        {
            character(j, ',');
        }
        signed_number(j, values[i]);
    }
    character(j, ']');
}
static void optional(Json *j, bool valid, int32_t value)
{
    if (valid)
    {
        signed_number(j, value);
    }
    else
    {
        text(j, "null");
    }
}
static void event_record(Json *j, const RunEvent *e)
{
    character(j, '[');
    number(j, e->id);
    character(j, ',');
    number(j, e->sampled_at);
    character(j, ',');
    number(j, e->revision);
    character(j, ',');
    signed_number(j, e->a);
    character(j, ',');
    signed_number(j, e->b);
    character(j, ',');
    signed_number(j, e->value);
    character(j, ',');
    number(j, e->detail);
    character(j, ']');
}
static void latest_event(Json *j, const RunHistory *h)
{
    if (h->length == 0u || h->length > RUN_HISTORY_CAPACITY)
    {
        text(j, "null");
    }
    else
    {
        event_record(j, &h->events[h->length - 1u]);
    }
}
unsigned s1_telemetry_encode(const S1Telemetry *s, uint32_t now,
                             char *destination, unsigned capacity)
{
    if (destination == NULL || capacity == 0u)
    {
        return 0u;
    }
    destination[0] = '\0';
    if (s == NULL)
    {
        return 0u;
    }
    if (capacity > S1_TELEMETRY_PAYLOAD_MAX + 1u)
    {
        capacity = S1_TELEMETRY_PAYLOAD_MAX + 1u;
    }
    Json j = {destination, 0u, capacity, true};
    bool line_valid = s->reading_valid && now - s->sampled_ms <= 60u;
    bool motion_valid = s->motion_valid && now - s->motion_ms <= 60u;
    bool terrain_valid = s->terrain_valid && now - s->terrain_ms <= 250u;
    bool front_valid = s->front_valid && now - s->front_ms <= 250u;
    text(&j, "{\"schema\":3,\"sequence\":");
    number(&j, s->sequence);
    text(&j, ",\"observed_ms\":");
    number(&j, s->observed_ms);
    text(&j, ",\"mission\":{\"state\":");
    quoted(&j, s->state, sizeof(s->state));
    text(&j, ",\"reason\":");
    quoted(&j, s->reason, sizeof(s->reason));
    text(&j, ",\"run\":");
    number(&j, s->run_epoch);
    text(&j, ",\"action\":");
    number(&j, s->action_id);
    text(&j, ",\"completed\":");
    number(&j, s->completed_actions);
    text(&j, ",\"configured\":");
    boolean(&j, s->configured);
    text(&j, ",\"moving\":");
    boolean(&j, s->motion_enabled);
    text(&j, ",\"stopped\":");
    boolean(&j, s->safety_latched);
    text(&j, ",\"test_inhibited\":");
    boolean(&j, s->test_inhibited);
    text(&j, "},\"line\":{\"ms\":");
    number(&j, s->sampled_ms);
    text(&j, ",\"age_ms\":");
    number(&j, now - s->sampled_ms);
    text(&j, ",\"valid\":");
    boolean(&j, line_valid);
    text(&j, ",\"raw\":");
    reference(&j, s->acquired, s->raw_left, s->raw_right);
    text(&j, ",\"black\":");
    reference(&j, s->black_set, s->black_left, s->black_right);
    text(&j, ",\"white\":");
    reference(&j, s->white_set, s->white_left, s->white_right);
    text(&j, ",\"fraction\":");
    if (line_valid)
    {
        character(&j, '[');
        decimal(&j, s->left_fraction);
        character(&j, ',');
        decimal(&j, s->right_fraction);
        character(&j, ']');
    }
    else
    {
        text(&j, "null");
    }
    text(&j, ",\"error\":");
    if (line_valid)
    {
        decimal(&j, s->error);
    }
    else
    {
        text(&j, "null");
    }
    text(&j, ",\"condition\":");
    quoted(&j, s->line, sizeof(s->line));
    text(&j, ",\"capture\":");
    quoted(&j, s->capture, sizeof(s->capture));
    text(&j, "},\"motion\":{\"ms\":");
    number(&j, s->motion_ms);
    text(&j, ",\"age_ms\":");
    number(&j, now - s->motion_ms);
    text(&j, ",\"valid\":");
    boolean(&j, motion_valid);
    text(&j, ",\"stationary\":");
    boolean(&j, motion_valid && s->stationary);
    text(&j, ",\"encoder_valid\":");
    boolean(&j, s->encoder_valid && now - s->motion_ms <= 60u);
    text(&j, ",\"count\":[");
    number(&j, s->left_count);
    character(&j, ',');
    number(&j, s->right_count);
    character(&j, ']');
    text(&j, ",\"distance_mm\":");
    integer_pair(&j, motion_valid, s->left_mm, s->right_mm);
    text(&j, ",\"speed_mm_s\":");
    integer_pair(&j, motion_valid, s->left_mmps, s->right_mmps);
    text(&j, ",\"xy_mm\":");
    integer_pair(&j, motion_valid, s->x_mm, s->y_mm);
    text(&j, ",\"yaw_mdeg\":");
    optional(&j, motion_valid, s->yaw_mdeg);
    text(&j, ",\"duty\":[");
    decimal(&j, s->output_left);
    character(&j, ',');
    decimal(&j, s->output_right);
    character(&j, ']');
    text(&j, ",\"status\":");
    number(&j, s->motion_status);
    text(&j, ",\"reason\":");
    number(&j, s->motion_reason);
    text(&j, "},\"terrain\":{\"ms\":");
    number(&j, s->terrain_ms);
    text(&j, ",\"age_ms\":");
    number(&j, now - s->terrain_ms);
    text(&j, ",\"valid\":");
    boolean(&j, terrain_valid);
    text(&j, ",\"calibrated\":");
    boolean(&j, s->terrain_calibrated);
    text(&j, ",\"accel_mg\":");
    integer_triple(&j, terrain_valid, s->accel_mg);
    text(&j, ",\"mag_milli_gauss\":");
    integer_triple(&j, s->magnetic_valid && now - s->magnetic_ms <= 250u,
                   s->magnetic_milli_gauss);
    text(&j, ",\"pitch_mdeg\":");
    optional(&j, terrain_valid, s->pitch_mdeg);
    text(&j, ",\"roll_mdeg\":");
    optional(&j, terrain_valid, s->roll_mdeg);
    text(&j, ",\"turn_mdeg_s\":");
    optional(&j, terrain_valid && motion_valid && s->terrain_motion_valid,
             s->turn_mdeg_s);
    text(&j, ",\"state\":");
    number(&j, s->terrain_state);
    text(&j, ",\"movement\":");
    optional(&j, terrain_valid && motion_valid && s->terrain_motion_valid,
             (int32_t)s->movement);
    text(&j, ",\"run\":");
    number(&j, s->terrain_run_epoch);
    text(&j, ",\"height_mm\":");
    optional(&j,
             terrain_valid && s->height_valid &&
                 s->terrain_run_epoch == s->run_epoch,
             s->height_mm);
    text(&j, ",\"max_height_mm\":");
    optional(&j, s->terrain_run_epoch == s->run_epoch && s->max_height_mm > 0,
             s->max_height_mm);
    text(&j, "},\"obstacle\":{\"ms\":");
    number(&j, s->front_ms);
    text(&j, ",\"age_ms\":");
    number(&j, now - s->front_ms);
    text(&j, ",\"front_mm\":");
    optional(&j, front_valid, s->front_mm);
    text(&j, ",\"profile_ms\":");
    number(&j, s->profile_ms);
    text(&j, ",\"profile_valid\":");
    boolean(&j, s->profile_valid);
    text(&j, ",\"width_mm\":");
    optional(&j, s->profile_valid, s->width_mm);
    text(&j, ",\"closest_mm\":");
    optional(&j, s->profile_valid, s->closest_mm);
    text(&j, ",\"gap_mm\":");
    integer_pair(&j, s->profile_valid, s->left_gap_mm, s->right_gap_mm);
    text(&j, ",\"scan_id\":");
    number(&j, s->scan_id);
    text(&j, ",\"scan_status\":");
    number(&j, s->scan_status);
    text(&j, "},\"barcode\":{\"symbol\":");
    quoted(&j, s->symbol, sizeof(s->symbol));
    text(&j, ",\"action\":");
    number(&j, s->navigation);
    text(&j, ",\"orientation\":");
    number(&j, s->orientation);
    text(&j, ",\"revision\":");
    number(&j, s->mapping_revision);
    text(&j, ",\"epoch\":");
    number(&j, s->scanner_epoch);
    text(&j, ",\"faults\":");
    number(&j, s->barcode_faults);
    text(&j, "},\"events\":{\"humps\":");
    number(&j, s->humps.count);
    text(&j, ",\"barcodes\":");
    number(&j, s->barcodes.count);
    text(&j, ",\"latest_hump\":");
    latest_event(&j, &s->humps);
    text(&j, ",\"latest_barcode\":");
    latest_event(&j, &s->barcodes);
    text(&j, ",\"dropped\":[");
    number(&j, s->humps.dropped);
    character(&j, ',');
    number(&j, s->barcodes.dropped);
    text(&j, "],\"overflow\":");
    boolean(&j, s->humps.overflow || s->barcodes.overflow);
    text(&j, "},\"calibration\":{\"status\":");
    number(&j, s->calibration_status);
    text(&j, ",\"samples\":");
    number(&j, s->calibration_samples);
    text(&j, ",\"reason\":");
    number(&j, s->calibration_reason);
    text(&j, "},\"mapping\":{\"request_id\":");
    number(&j, s->mapping_request_id);
    text(&j, ",\"result\":");
    number(&j, s->mapping_result);
    text(&j, "}}");
    if (!j.ok)
    {
        destination[0] = '\0';
        return 0u;
    }
    destination[j.length] = '\0';
    return j.length;
}

unsigned s1_robot_status_encode(const S1Telemetry *s, uint32_t now, char *out,
                                unsigned capacity)
{
    static const char prefix[] = "{\"online\":true,";
    unsigned extra = sizeof(prefix) - 2u;
    if (out == NULL || capacity <= extra + 1u)
    {
        return 0u;
    }
    if (capacity > S1_TELEMETRY_PAYLOAD_MAX + 1u)
    {
        capacity = S1_TELEMETRY_PAYLOAD_MAX + 1u;
    }
    unsigned length = s1_telemetry_encode(s, now, out, capacity - extra);
    if (length == 0u)
    {
        return 0u;
    }
    for (unsigned i = length; i > 0u; --i)
    {
        out[i + extra] = out[i];
    }
    for (unsigned i = 0u; i < sizeof(prefix) - 1u; ++i)
    {
        out[i] = prefix[i];
    }
    return length + extra;
}
unsigned s1_events_encode(const RunHistory *h, bool barcode, uint32_t after,
                          char *out, unsigned capacity)
{
    if (out == NULL || capacity == 0u)
    {
        return 0u;
    }
    out[0] = '\0';
    if (h == NULL || h->length > RUN_HISTORY_CAPACITY)
    {
        return 0u;
    }
    if (capacity > S1_TELEMETRY_PAYLOAD_MAX + 1u)
    {
        capacity = S1_TELEMETRY_PAYLOAD_MAX + 1u;
    }
    Json j = {out, 0u, capacity, true};
    text(&j, "{\"schema\":3,\"kind\":");
    text(&j, barcode ? "\"barcode\"" : "\"hump\"");
    text(&j, ",\"run\":");
    number(&j, h->run_epoch);
    text(&j, ",\"after_id\":");
    number(&j, after);
    text(&j, ",\"count\":");
    number(&j, h->count);
    text(&j, ",\"dropped\":");
    number(&j, h->dropped);
    text(&j, ",\"overflow\":");
    boolean(&j, h->overflow);
    text(&j, ",\"events\":[");
    bool comma = false;
    for (unsigned i = 0u; i < h->length; ++i)
    {
        const RunEvent *e = &h->events[i];
        if (e->id <= after)
        {
            continue;
        }
        if (comma)
        {
            character(&j, ',');
        }
        comma = true;
        event_record(&j, e);
    }
    text(&j, "]}");
    if (!j.ok)
    {
        out[0] = '\0';
        return 0u;
    }
    out[j.length] = '\0';
    return j.length;
}
