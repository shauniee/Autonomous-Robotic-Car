#include "telemetry.h"

typedef struct { char *data; unsigned length, capacity; bool ok; } Json;
static void character(Json *j, char c) {
    if (j->length + 1 >= j->capacity) { j->ok = false; return; }
    j->data[j->length++] = c;
}
static void text(Json *j, const char *s) {
    while (*s && j->ok) character(j, *s++);
}
static void number(Json *j, uint32_t n) {
    char digits[10]; unsigned count = 0;
    do { digits[count++] = (char)('0' + n % 10); n /= 10; } while (n);
    while (count) character(j, digits[--count]);
}
static void boolean(Json *j, bool b) { text(j, b ? "true" : "false"); }
static void decimal(Json *j, int16_t value) {
    int32_t n = value;
    if (n < 0) { character(j, '-'); n = -n; }
    number(j, (uint32_t)n / 1000u); character(j, '.');
    character(j, (char)('0' + (n / 100) % 10));
    character(j, (char)('0' + (n / 10) % 10));
    character(j, (char)('0' + n % 10));
}
static void signed_milli(Json *j, int32_t value)
{
    uint32_t magnitude = value < 0 ? 0u - (uint32_t)value : (uint32_t)value;
    if (value < 0)
    {
        character(j, '-');
    }
    number(j, magnitude / 1000u);
    character(j, '.');
    character(j, (char)('0' + magnitude / 100u % 10u));
    character(j, (char)('0' + magnitude / 10u % 10u));
    character(j, (char)('0' + magnitude % 10u));
}
static void quoted(Json *j, const char *s, unsigned capacity) {
    static const char hex[] = "0123456789abcdef";
    character(j, '"');
    for (unsigned i = 0; i < capacity; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (!c) { character(j, '"'); return; }
        if (c == '"' || c == '\\') { character(j, '\\'); character(j, (char)c); }
        else if (c < 32) {
            text(j, "\\u00"); character(j, hex[c / 16]); character(j, hex[c % 16]);
        } else character(j, (char)c);
    }
    j->ok = false; /* Unterminated public input is rejected, never read beyond. */
}
static void reference(Json *j, bool valid, uint16_t left, uint16_t right) {
    if (!valid) { text(j, "null"); return; }
    text(j, "{\"left\":"); number(j, left);
    text(j, ",\"right\":"); number(j, right); character(j, '}');
}
unsigned s1_telemetry_encode(const S1Telemetry *s, uint32_t now,
                             char *destination, unsigned capacity) {
    if (!destination || !capacity) return 0;
    destination[0] = '\0';
    if (!s) return 0;
    if (capacity > S1_TELEMETRY_PAYLOAD_MAX + 1) capacity = S1_TELEMETRY_PAYLOAD_MAX + 1;
    Json j = {destination, 0, capacity, true};
    text(&j, "{\"schema\":1,\"sequence\":"); number(&j, s->sequence);
    text(&j, ",\"ir_sequence\":");
    number(&j, s->ir_sequence);
    text(&j, ",\"observed_ms\":"); number(&j, s->observed_ms);
    text(&j, ",\"sampled_ms\":");
    if (s->have_sample) number(&j, s->sampled_ms); else text(&j, "null");
    text(&j, ",\"sample_age_ms\":");
    if (s->have_sample) number(&j, (uint32_t)(now - s->sampled_ms)); else text(&j, "null");
    text(&j, ",\"acquisition_valid\":"); boolean(&j, s->have_sample && s->acquired);
    text(&j, ",\"sample_fresh\":");
    boolean(&j, s->have_sample && s->acquired && (uint32_t)(now - s->sampled_ms) <= 60u);
    text(&j, ",\"raw\":"); reference(&j, s->have_sample && s->acquired, s->raw_left, s->raw_right);
    text(&j, ",\"calibration\":{\"black_set\":"); boolean(&j, s->black_set);
    text(&j, ",\"white_set\":"); boolean(&j, s->white_set);
    text(&j, ",\"black\":"); reference(&j, s->black_set, s->black_left, s->black_right);
    text(&j, ",\"white\":"); reference(&j, s->white_set, s->white_left, s->white_right);
    text(&j, "},\"fractions\":");
    if (s->reading_valid) {
        text(&j, "{\"left\":"); decimal(&j, s->left_fraction);
        text(&j, ",\"right\":"); decimal(&j, s->right_fraction); character(&j, '}');
    } else text(&j, "null");
    text(&j, ",\"error\":");
    if (s->reading_valid) decimal(&j, s->error); else text(&j, "null");
    text(&j, ",\"line\":"); quoted(&j, s->line, sizeof(s->line));
    text(&j, ",\"state\":"); quoted(&j, s->state, sizeof(s->state));
    text(&j, ",\"reason\":"); quoted(&j, s->reason, sizeof(s->reason));
    text(&j, ",\"capture\":"); quoted(&j, s->capture, sizeof(s->capture));
    text(&j, ",\"requested_duty\":{\"left\":"); decimal(&j, s->left_duty);
    text(&j, ",\"right\":"); decimal(&j, s->right_duty);
    text(&j, "},\"motion_enabled\":"); boolean(&j, s->motion_enabled);
    text(&j, ",\"test_inhibited\":");
    boolean(&j, s->test_inhibited);
    text(&j, ",\"motion\":");
    if (s->motion_available)
    {
        text(&j, "{\"age_ms\":");
        number(&j, (uint32_t)(now - s->motion_serviced_ms));
        text(&j, ",\"stopped\":");
        boolean(&j, s->motion_stopped);
        text(&j, ",\"braked\":");
        boolean(&j, s->motion_braked);
        text(&j, ",\"applied_duty\":{\"left\":");
        decimal(&j, s->applied_left);
        text(&j, ",\"right\":");
        decimal(&j, s->applied_right);
        text(&j, "}}");
    }
    else
    {
        text(&j, "null");
    }
    text(&j, ",\"velocity_mode\":");
    boolean(&j, s->velocity_mode);
    text(&j, ",\"requested_speed_mps\":");
    if (s->speed_target_valid)
    {
        text(&j, "{\"left\":"); signed_milli(&j, s->target_left_milli);
        text(&j, ",\"right\":"); signed_milli(&j, s->target_right_milli);
        character(&j, '}');
    }
    else
    {
        text(&j, "null");
    }
    text(&j, ",\"encoders\":");
    if (s->motion_available)
    {
        text(&j, "{\"valid\":"); boolean(&j, s->encoder_valid);
        text(&j, ",\"left_count\":"); number(&j, s->encoder_left_count);
        text(&j, ",\"right_count\":"); number(&j, s->encoder_right_count);
        text(&j, ",\"errors\":"); number(&j, s->encoder_errors);
        text(&j, ",\"speed_mps\":");
        if (s->encoder_calibrated &&
            ((uint32_t)(now - s->navigation_sampled_ms) <= 60u))
        {
            text(&j, "{\"left\":"); signed_milli(&j, s->speed_left_milli);
            text(&j, ",\"right\":"); signed_milli(&j, s->speed_right_milli);
            character(&j, '}');
        }
        else
        {
            text(&j, "null");
        }
        character(&j, '}');
    }
    else
    {
        text(&j, "null");
    }
    text(&j, ",\"navigation\":{\"id\":"); number(&j, s->action_id);
    text(&j, ",\"state\":"); number(&j, s->action_state);
    text(&j, ",\"age_ms\":");
    if (s->encoder_calibrated)
    {
        number(&j, (uint32_t)(now - s->navigation_sampled_ms));
    }
    else
    {
        text(&j, "null");
    }
    text(&j, "},\"barcode\":");
    if (s->barcode_available)
    {
        text(&j, "{\"state\":");
        quoted(&j, s->barcode_state, sizeof(s->barcode_state));
        text(&j, ",\"result\":");
        quoted(&j, s->barcode_result, sizeof(s->barcode_result));
        text(&j, ",\"symbol\":");
        quoted(&j, s->barcode_symbol, sizeof(s->barcode_symbol));
        text(&j, ",\"sequence\":"); number(&j, s->barcode_sequence);
        text(&j, ",\"queued\":"); number(&j, s->barcode_queued);
        text(&j, ",\"accepted\":"); number(&j, s->barcode_accepted);
        text(&j, ",\"rejected\":"); number(&j, s->barcode_rejected);
        text(&j, ",\"discarded\":"); number(&j, s->barcode_discarded);
        text(&j, ",\"age_ms\":");
        number(&j, (uint32_t)(now - s->barcode_serviced_ms));
        character(&j, '}');
    }
    else
    {
        text(&j, "null");
    }
    character(&j, '}');
    if (!j.ok) { destination[0] = '\0'; return 0; }
    destination[j.length] = '\0'; return j.length;
}
