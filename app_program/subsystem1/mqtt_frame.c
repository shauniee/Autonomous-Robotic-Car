#include "mqtt_frame.h"

#define HDR(type) ((uint8_t)((MQTT_FRAME_VERSION << 4) | ((unsigned)(type) & 0xFu)))

static unsigned type_length(unsigned type)
{
    switch (type)
    {
    case MQTT_TYPE_TELEMETRY:
    case MQTT_TYPE_SNAPSHOT_REPLY: return MQTT_LEN_TELEMETRY;
    case MQTT_TYPE_EVENT: return MQTT_LEN_EVENT;
    case MQTT_TYPE_MAPPING: return MQTT_LEN_MAPPING;
    case MQTT_TYPE_ACK: return MQTT_LEN_ACK;
    case MQTT_TYPE_SNAPSHOT_REQUEST: return MQTT_LEN_SNAPSHOT_REQUEST;
    default: return 0u;
    }
}

mqtt_decode_t mqtt_frame_type(const uint8_t *f, unsigned length, mqtt_type_t *type)
{
    if (f == 0 || type == 0 || length == 0u) return MQTT_DECODE_NULL;
    if ((unsigned)(f[0] >> 4) != MQTT_FRAME_VERSION) return MQTT_DECODE_VERSION;
    unsigned t = f[0] & 0xFu, expect = type_length(t);
    if (expect == 0u) return MQTT_DECODE_TYPE;
    if (length != expect) return MQTT_DECODE_LENGTH;
    *type = (mqtt_type_t)t;
    return MQTT_DECODE_OK;
}

static void put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v & 0xFFu); p[1] = (uint8_t)(v >> 8); }
static uint16_t get16(const uint8_t *p) { return (uint16_t)(p[0] | ((unsigned)p[1] << 8)); }

/* ---- Telemetry -----------------------------------------------------------
   0 header | 1 sequence | 2 [3:0] mission state, [5:4] motion status, [7:6] line status
   3 [5:0] motion action, [6] mapped, [7] 0 | 4..9 fault bytes | 10..11 left | 12..13 right
   14 hump count | 15 [6:0] hump max, [7] hump valid
   16..17 [12:0] front mm, [13] front valid, [15:14] 0
   18 [4:0] barcode letter, [7:5] barcode action | 19..20 obstacle width mm | 21..22 obstacle depth mm */

unsigned mqtt_encode_telemetry(uint8_t *out, unsigned capacity, mqtt_type_t type, const mqtt_telemetry_t *t)
{
    if (out == 0 || t == 0 || capacity < MQTT_LEN_TELEMETRY ||
        (type != MQTT_TYPE_TELEMETRY && type != MQTT_TYPE_SNAPSHOT_REPLY) ||
        t->mission_state > 15u || t->motion_status > 3u || t->line_status > 3u || t->motion_action > 63u ||
        t->hump_max > 127u || t->front_mm > 8191u || t->barcode_letter > 31u || t->barcode_action > 7u)
    {
        return 0u;
    }
    out[0] = HDR(type);
    out[1] = t->sequence;
    out[2] = (uint8_t)(t->mission_state | (t->motion_status << 4) | (t->line_status << 6));
    out[3] = (uint8_t)(t->motion_action | (t->mapped ? 0x40u : 0u));
    for (unsigned i = 0u; i < MQTT_FAULT_BYTES; ++i) out[4u + i] = t->faults[i];
    put16(&out[10], t->left_count);
    put16(&out[12], t->right_count);
    out[14] = t->hump_count;
    out[15] = (uint8_t)(t->hump_max | (t->hump_valid ? 0x80u : 0u));
    put16(&out[16], (uint16_t)(t->front_mm | (t->front_valid ? 0x2000u : 0u)));
    out[18] = (uint8_t)(t->barcode_letter | (t->barcode_action << 5));
    put16(&out[19], t->obstacle_width_mm);
    put16(&out[21], t->obstacle_depth_mm);
    return MQTT_LEN_TELEMETRY;
}

mqtt_decode_t mqtt_decode_telemetry(const uint8_t *f, unsigned length, mqtt_type_t *type, mqtt_telemetry_t *t)
{
    mqtt_type_t ty;
    mqtt_decode_t r = mqtt_frame_type(f, length, &ty);
    if (r != MQTT_DECODE_OK) return r;
    if (t == 0 || type == 0) return MQTT_DECODE_NULL;
    if (ty != MQTT_TYPE_TELEMETRY && ty != MQTT_TYPE_SNAPSHOT_REPLY) return MQTT_DECODE_TYPE;
    if ((f[3] & 0x80u) != 0u || (f[17] & 0xC0u) != 0u) return MQTT_DECODE_RESERVED;
    uint8_t letter = f[18] & 0x1Fu;
    if (letter > 25u && letter != MQTT_NO_LETTER) return MQTT_DECODE_RANGE;
    *type = ty;
    t->sequence = f[1];
    t->mission_state = f[2] & 0xFu;
    t->motion_status = (f[2] >> 4) & 0x3u;
    t->line_status = f[2] >> 6;
    t->motion_action = f[3] & 0x3Fu;
    t->mapped = (f[3] & 0x40u) != 0u;
    for (unsigned i = 0u; i < MQTT_FAULT_BYTES; ++i) t->faults[i] = f[4u + i];
    t->left_count = get16(&f[10]);
    t->right_count = get16(&f[12]);
    t->hump_count = f[14];
    t->hump_max = f[15] & 0x7Fu;
    t->hump_valid = (f[15] & 0x80u) != 0u;
    uint16_t front = get16(&f[16]);
    t->front_mm = front & 0x1FFFu;
    t->front_valid = (front & 0x2000u) != 0u;
    t->barcode_letter = letter;
    t->barcode_action = f[18] >> 5;
    t->obstacle_width_mm = get16(&f[19]);
    t->obstacle_depth_mm = get16(&f[21]);
    return MQTT_DECODE_OK;
}

/* ---- Event --------------------------------------------------------------
   0 header | 1 [3:0] kind, [7:4] 0 | 2 arg | 3..4 time | 5..6 left | 7..8 right | 9..10 arg2 */

unsigned mqtt_encode_event(uint8_t *out, unsigned capacity, const mqtt_event_t *e)
{
    if (out == 0 || e == 0 || capacity < MQTT_LEN_EVENT || e->kind == 0u || e->kind >= MQTT_EVENT_COUNT)
        return 0u;
    out[0] = HDR(MQTT_TYPE_EVENT);
    out[1] = (uint8_t)(e->kind & 0xFu);
    out[2] = e->arg;
    put16(&out[3], e->time_ms);
    put16(&out[5], e->left_count);
    put16(&out[7], e->right_count);
    put16(&out[9], e->arg2);
    return MQTT_LEN_EVENT;
}

mqtt_decode_t mqtt_decode_event(const uint8_t *f, unsigned length, mqtt_event_t *e)
{
    mqtt_type_t ty;
    mqtt_decode_t r = mqtt_frame_type(f, length, &ty);
    if (r != MQTT_DECODE_OK) return r;
    if (e == 0) return MQTT_DECODE_NULL;
    if (ty != MQTT_TYPE_EVENT) return MQTT_DECODE_TYPE;
    if ((f[1] & 0xF0u) != 0u) return MQTT_DECODE_RESERVED;
    if ((f[1] & 0xFu) == 0u || (f[1] & 0xFu) >= MQTT_EVENT_COUNT) return MQTT_DECODE_RANGE;
    e->kind = f[1] & 0xFu;
    e->arg = f[2];
    e->time_ms = get16(&f[3]);
    e->left_count = get16(&f[5]);
    e->right_count = get16(&f[7]);
    e->arg2 = get16(&f[9]);
    return MQTT_DECODE_OK;
}

/* ---- Mapping ------------------------------------------------------------
   0 header | 1 sequence | 2..4 little-endian 24 bits: 4 x 5-bit letters in
   bits 0-19 (left in bits 0-4), bits 20-23 zero                              */

static bool letters_ok(const uint8_t l[4])
{
    uint32_t seen = 0u;
    for (unsigned i = 0u; i < 4u; ++i)
    {
        if (l[i] > 25u || (seen & (1u << l[i])) != 0u) return false;
        seen |= 1u << l[i];
    }
    return true;
}

unsigned mqtt_encode_mapping(uint8_t *out, unsigned capacity, const mqtt_mapping_t *m)
{
    if (out == 0 || m == 0 || capacity < MQTT_LEN_MAPPING || !letters_ok(m->letters)) return 0u;
    uint32_t packed = 0u;
    for (unsigned i = 0u; i < 4u; ++i) packed |= (uint32_t)m->letters[i] << (5u * i);
    out[0] = HDR(MQTT_TYPE_MAPPING);
    out[1] = m->sequence;
    out[2] = (uint8_t)(packed & 0xFFu);
    out[3] = (uint8_t)((packed >> 8) & 0xFFu);
    out[4] = (uint8_t)((packed >> 16) & 0xFFu);
    return MQTT_LEN_MAPPING;
}

mqtt_decode_t mqtt_decode_mapping(const uint8_t *f, unsigned length, mqtt_mapping_t *m)
{
    mqtt_type_t ty;
    mqtt_decode_t r = mqtt_frame_type(f, length, &ty);
    if (r != MQTT_DECODE_OK) return r;
    if (m == 0) return MQTT_DECODE_NULL;
    if (ty != MQTT_TYPE_MAPPING) return MQTT_DECODE_TYPE;
    uint32_t packed = (uint32_t)f[2] | ((uint32_t)f[3] << 8) | ((uint32_t)f[4] << 16);
    if ((packed >> 20) != 0u) return MQTT_DECODE_RESERVED;
    uint8_t letters[4];
    for (unsigned i = 0u; i < 4u; ++i) letters[i] = (uint8_t)((packed >> (5u * i)) & 0x1Fu);
    if (!letters_ok(letters)) return MQTT_DECODE_RANGE;
    m->sequence = f[1];
    for (unsigned i = 0u; i < 4u; ++i) m->letters[i] = letters[i];
    return MQTT_DECODE_OK;
}

/* ---- Ack and snapshot request ---------------------------------------- */

unsigned mqtt_encode_ack(uint8_t *out, unsigned capacity, const mqtt_ack_t *a)
{
    if (out == 0 || a == 0 || capacity < MQTT_LEN_ACK || a->result > MQTT_ACK_MALFORMED) return 0u;
    out[0] = HDR(MQTT_TYPE_ACK);
    out[1] = a->sequence;
    out[2] = a->result;
    return MQTT_LEN_ACK;
}

mqtt_decode_t mqtt_decode_ack(const uint8_t *f, unsigned length, mqtt_ack_t *a)
{
    mqtt_type_t ty;
    mqtt_decode_t r = mqtt_frame_type(f, length, &ty);
    if (r != MQTT_DECODE_OK) return r;
    if (a == 0) return MQTT_DECODE_NULL;
    if (ty != MQTT_TYPE_ACK) return MQTT_DECODE_TYPE;
    if (f[2] > MQTT_ACK_MALFORMED) return MQTT_DECODE_RANGE;
    a->sequence = f[1];
    a->result = f[2];
    return MQTT_DECODE_OK;
}

unsigned mqtt_encode_snapshot_request(uint8_t *out, unsigned capacity, uint8_t sequence)
{
    if (out == 0 || capacity < MQTT_LEN_SNAPSHOT_REQUEST) return 0u;
    out[0] = HDR(MQTT_TYPE_SNAPSHOT_REQUEST);
    out[1] = sequence;
    return MQTT_LEN_SNAPSHOT_REQUEST;
}

mqtt_decode_t mqtt_decode_snapshot_request(const uint8_t *f, unsigned length, uint8_t *sequence)
{
    mqtt_type_t ty;
    mqtt_decode_t r = mqtt_frame_type(f, length, &ty);
    if (r != MQTT_DECODE_OK) return r;
    if (sequence == 0) return MQTT_DECODE_NULL;
    if (ty != MQTT_TYPE_SNAPSHOT_REQUEST) return MQTT_DECODE_TYPE;
    *sequence = f[1];
    return MQTT_DECODE_OK;
}
