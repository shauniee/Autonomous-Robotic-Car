#include "robot/comms_protocol.h"
#include "robot/controller.h"
#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* A deliberately small, strict JSON subset: exactly four known fields,
   unsigned decimal integers and unescaped ASCII strings, in any order. */
/* Parsing means reading text and turning it into values the program understands.
   'p' is our current position in that text. Passing 'const char **p' allows a
   helper to advance the caller's position; **p is the character at that position.
   These helpers return false as soon as the expected format is not present. */
static void whitespace(const char **p) {
    while (**p == ' ' || **p == '\t' || **p == '\r' || **p == '\n') ++*p;
}

static bool token(const char **p, char expected) {
    /* Consume one punctuation character, such as '{', ':' or ',', skipping spaces. */
    whitespace(p);
    if (**p != expected) return false;
    ++*p;
    return true;
}

static bool string_value(const char **p, char *out, size_t capacity) {
    /* Read text between quotes. Reserve space for '\0', the C end-of-string marker.
       The command format intentionally disallows escaped/non-ASCII strings. */
    if (!token(p, '"')) return false;
    size_t n = 0;
    while (**p != '"') {
        unsigned char c = (unsigned char)**p;
        if (c < 32 || c > 126 || c == '\\' || n + 1 >= capacity) return false;
        out[n++] = *(*p)++;
    }
    ++*p;
    out[n] = '\0';
    return true;
}

static bool uint_value(const char **p, uint32_t *value) {
    /* Build a non-negative integer one digit at a time; reject JSON leading zeros. */
    whitespace(p);
    if (**p < '0' || **p > '9') return false;
    if (**p == '0' && (*p)[1] >= '0' && (*p)[1] <= '9') return false;
    uint32_t v = 0;
    do {
        const uint32_t digit = (uint32_t)(**p - '0');
        /* Check before multiplying, so an oversized number cannot wrap around. */
        if (v > (UINT32_MAX - digit) / 10u) return false;
        v = v * 10u + digit;
        ++*p;
    } while (**p >= '0' && **p <= '9');
    *value = v;
    return true;
}

static bool accept_command(CommandInbox *inbox, uint32_t now_ms) {
    /* 'inbox->field' accesses a field through a pointer to an existing struct. */
    /* Plain stop is also supported: it can only stop, never start a vehicle. */
    if (strcmp(inbox->payload, "stop") == 0) {
        command_inbox_stop(inbox);
        return true;
    }
    const char *p = inbox->payload;
    char session[COMMS_SESSION_LENGTH + 1] = {0};
    char command[8] = {0};
    uint32_t id = 0, issued_ms = 0;
    unsigned fields = 0; /* Bit mask: session=1, id=2, issued_ms=4, command=8. */
    if (!token(&p, '{')) return false;
    for (int i = 0; i < 4; ++i) {
        char key[16];
        if (i && !token(&p, ',')) return false;
        if (!string_value(&p, key, sizeof(key)) || !token(&p, ':')) return false;
        unsigned bit;
        if (strcmp(key, "session") == 0) {
            bit = 1;
            if (!string_value(&p, session, sizeof(session))) return false;
        } else if (strcmp(key, "id") == 0) {
            bit = 2;
            if (!uint_value(&p, &id)) return false;
        } else if (strcmp(key, "issued_ms") == 0) {
            bit = 4;
            if (!uint_value(&p, &issued_ms)) return false;
        } else if (strcmp(key, "command") == 0) {
            bit = 8;
            if (!string_value(&p, command, sizeof(command))) return false;
        } else return false;
        if (fields & bit) return false; /* A set bit means this field already appeared. */
        fields |= bit;                 /* Remember that this field has now appeared. */
    }
    if (!token(&p, '}')) return false;
    whitespace(&p);
    /* Require end-of-input, all four bits (1+2+4+8=15), the current session,
       an increasing ID and a recent board-uptime timestamp. */
    if (*p || fields != 15 || !inbox->session[0] ||
        strcmp(session, inbox->session) != 0 || id <= inbox->last_id ||
        (uint32_t)(now_ms - issued_ms) > COMMS_COMMAND_MAX_AGE_MS) return false;
    RobotCommand result;
    if (strcmp(command, "start") == 0) result = COMMAND_START;
    else if (strcmp(command, "stop") == 0) result = COMMAND_STOP;
    else return false;
    inbox->last_id = id;
    /* A new START must never replace a STOP waiting for the controller. */
    if (inbox->pending != COMMAND_STOP) {
        inbox->pending = result;
        inbox->pending_issued_ms = issued_ms;
    }
    return true;
}

void command_inbox_reset(CommandInbox *inbox, const char *session) {
    /* A zero-initialized struct clears counters, fragments and pending commands. */
    *inbox = (CommandInbox){0};
    if (strlen(session) == COMMS_SESSION_LENGTH) {
        memcpy(inbox->session, session, COMMS_SESSION_LENGTH + 1);
    }
}

void command_inbox_stop(CommandInbox *inbox) {
    inbox->pending = COMMAND_STOP;
}

void command_inbox_begin(CommandInbox *inbox, size_t length, bool correct_topic) {
    inbox->received = 0;
    inbox->expected = length;
    inbox->receiving = correct_topic && length > 0 && length <= COMMS_COMMAND_MAX;
}

bool command_inbox_data(CommandInbox *inbox, const void *data, size_t length,
                        bool last, uint32_t now_ms) {
    if (!inbox->receiving) return false;
    /* Reject too many bytes, missing fragment data and embedded string terminators.
       An embedded '\0' could otherwise hide trailing text from the parser. */
    if (length > inbox->expected - inbox->received ||
        (length && (!data || memchr(data, '\0', length)))) {
        inbox->receiving = false;
        return false;
    }
    /* Append at the first unused byte; do not overwrite earlier fragments. */
    if (length) memcpy(inbox->payload + inbox->received, data, length);
    inbox->received += length;
    if (!last) return true; /* Fragment accepted; complete command not validated yet. */
    inbox->receiving = false;
    if (inbox->received != inbox->expected) return false;
    inbox->payload[inbox->received] = '\0';
    return accept_command(inbox, now_ms);
}

RobotCommand command_inbox_take(CommandInbox *inbox, uint32_t now_ms) {
    RobotCommand command = inbox->pending;
    inbox->pending = COMMAND_NONE; /* Clear it now so the next loop cannot repeat it. */
    if (command == COMMAND_START &&
        (uint32_t)(now_ms - inbox->pending_issued_ms) > COMMS_COMMAND_MAX_AGE_MS) {
        return COMMAND_NONE;
    }
    return command;
}

/* Tracks a fixed output buffer: its address, total space, used bytes and overflow.
   Encoding is the reverse of parsing: convert values into JSON text for MQTT. */
typedef struct { char *out; size_t capacity, used; bool failed; } JsonWriter;

static void append(JsonWriter *w, const char *format, ...) {
    /* '...' accepts printf-style arguments. va_list passes them to vsnprintf,
       which writes only within the remaining space in the output buffer. */
    if (w->failed) return;
    va_list args;
    va_start(args, format);
    const int n = vsnprintf(w->out + w->used, w->capacity - w->used, format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= w->capacity - w->used) w->failed = true;
    else w->used += (size_t)n;
}

static void quoted(JsonWriter *w, const char *s) {
    /* Surround text with JSON quotes and escape quote/backslash/control bytes
       so a reason string cannot accidentally break the JSON message. */
    append(w, "\"");
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') append(w, "\\%c", c);
        else if (c < 32 || c > 126) append(w, "\\u%04x", (unsigned)c);
        else append(w, "%c", c);
    }
    append(w, "\"");
}

static void number(JsonWriter *w, const char *name, float value, bool valid) {
    /* NaN and infinity are not JSON numbers. Send null for those or invalid data,
       rather than a misleading zero. %.6g keeps up to six significant digits. */
    append(w, ",\"%s\":", name);
    if (valid && isfinite(value)) append(w, "%.6g", (double)value);
    else append(w, "null");
}

static void sample(JsonWriter *w, const char *name, SampleStatus status, uint32_t now) {
    /* Start a nested sensor object with validity and timing; the caller adds
       measurements and closes its brace. Age tells the laptop how old data is. */
    append(w, ",\"%s\":{\"valid\":%s,\"sampled_ms\":%" PRIu32 ",\"age_ms\":%" PRIu32,
           name, status.valid ? "true" : "false", status.sampled_ms,
           (uint32_t)(now - status.sampled_ms));
}

size_t comms_encode_telemetry(char *out, size_t capacity, const VehicleController *c,
                             const SensorFrame *s, const MotionFeedback *m, uint32_t now) {
    /* Build one snapshot. All helpers share the same writer and capacity checks.
       Target speeds come from the controller; measured speeds come from encoders. */
    if (!capacity) return 0;
    JsonWriter w = {out, capacity, 0, false};
    append(&w, "{\"schema\":1,\"uptime_ms\":%" PRIu32 ",\"state\":", now);
    quoted(&w, controller_state_name(c->state));
    append(&w, ",\"reason\":");
    quoted(&w, c->reason ? c->reason : "");
    number(&w, "target_left_mps", c->target.left_mps, true);
    number(&w, "target_right_mps", c->target.right_mps, true);
    sample(&w, "motion", m->status, now); /* Buddy 2: actual motion measurements. */
    number(&w, "left_mps", m->left_mps, m->status.valid);
    number(&w, "right_mps", m->right_mps, m->status.valid);
    number(&w, "distance_m", m->distance_m, m->status.valid);
    append(&w, "}");
    sample(&w, "line", s->line.status, now); /* Buddy 3: line and navigation data. */
    append(&w, ",\"detected\":%s,\"junction\":%s,\"navigation\":%d",
           s->line.detected ? "true" : "false", s->line.junction ? "true" : "false",
           (int)s->line.navigation);
    number(&w, "error", s->line.error, s->line.status.valid);
    append(&w, "}");
    sample(&w, "terrain", s->terrain.status, now); /* Buddy 4: IMU and hump data. */
    append(&w, ",\"collision\":%s,\"hump_detected\":%s,\"height_valid\":%s",
           s->terrain.collision ? "true" : "false",
           s->terrain.hump_detected ? "true" : "false",
           s->terrain.status.valid && s->terrain.height_valid ? "true" : "false");
    number(&w, "pitch_deg", s->terrain.pitch_deg, s->terrain.status.valid);
    number(&w, "yaw_rate_dps", s->terrain.yaw_rate_dps, s->terrain.status.valid);
    number(&w, "hump_height_m", s->terrain.hump_height_m,
           s->terrain.status.valid && s->terrain.height_valid);
    append(&w, "}");
    sample(&w, "obstacle", s->obstacle.status, now); /* Buddy 5: range and scan profile. */
    number(&w, "front_distance_m", s->obstacle.front_distance_m, s->obstacle.status.valid);
    const bool profile = s->obstacle.status.valid && s->obstacle.profile_valid;
    append(&w, ",\"profile_valid\":%s", profile ? "true" : "false");
    number(&w, "width_m", s->obstacle.obstacle_width_m, profile);
    number(&w, "left_clearance_m", s->obstacle.left_clearance_m, profile);
    number(&w, "right_clearance_m", s->obstacle.right_clearance_m, profile);
    append(&w, "}}");
    /* Never return a partial JSON message as usable telemetry. */
    if (w.failed) { out[0] = '\0'; return 0; }
    return w.used;
}
