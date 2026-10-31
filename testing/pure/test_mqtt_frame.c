/* Host test for app_program/subsystem1/mqtt_frame.[ch] (issue #39). The byte
   vectors below are the schema v2 golden vectors in docs/architecture.md;
   scripts/mqtt_client.py --self-test decodes the same bytes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../../app_program/subsystem1/mqtt_frame.h"

static const uint8_t golden_telemetry[MQTT_LEN_TELEMETRY] = {
    0x21, 0x2A, 0x72, 0x45, 0x01, 0x02, 0x04, 0x08, 0x01, 0x03, 0x34, 0x12, 0x78, 0x56,
    0x03, 0xF8, 0xD0, 0x30, 0x61, 0xA4, 0x01, 0xAA, 0x00};
static const uint8_t golden_event[MQTT_LEN_EVENT] = {0x22, 0x01, 0x19, 0xE8, 0x03, 0x10, 0x00, 0x11, 0x00, 0x02, 0x00};
static const uint8_t golden_mapping[MQTT_LEN_MAPPING] = {0x24, 0x07, 0x20, 0x88, 0x01};
static const uint8_t golden_ack[MQTT_LEN_ACK] = {0x25, 0x07, 0x00};
static const uint8_t golden_request[MQTT_LEN_SNAPSHOT_REQUEST] = {0x26, 0x09};

static mqtt_telemetry_t sample(void)
{
    mqtt_telemetry_t t = {.sequence = 0x2A, .mission_state = 2, .motion_status = 3, .motion_action = 5,
                          .line_status = 1, .mapped = true,
                          .faults = {1, 2, 4, 8, 1, 3}, .left_count = 0x1234, .right_count = 0x5678,
                          .hump_count = 3, .hump_max = 0x78, .hump_valid = true,
                          .front_mm = 0x10D0, .front_valid = true, .barcode_letter = 1, .barcode_action = 3,
                          .obstacle_width_mm = 420, .obstacle_depth_mm = 170};
    return t;
}

static void test_golden(void)
{
    uint8_t buf[MQTT_FRAME_MAX];
    mqtt_telemetry_t t = sample();
    assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &t) == MQTT_LEN_TELEMETRY);
    assert(memcmp(buf, golden_telemetry, MQTT_LEN_TELEMETRY) == 0);

    mqtt_type_t ty; mqtt_telemetry_t d;
    assert(mqtt_decode_telemetry(golden_telemetry, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_OK);
    assert(ty == MQTT_TYPE_TELEMETRY && memcmp(&d.faults, &t.faults, sizeof t.faults) == 0);
    assert(d.sequence == 0x2A && d.mission_state == 2 && d.motion_status == 3 && d.motion_action == 5);
    assert(d.left_count == 0x1234 && d.right_count == 0x5678 && d.hump_count == 3 && d.hump_max == 0x78);
    assert(d.hump_valid && d.front_mm == 0x10D0 && d.front_valid && d.barcode_letter == 1 && d.barcode_action == 3);
    assert(d.line_status == 1 && d.mapped && d.obstacle_width_mm == 420 && d.obstacle_depth_mm == 170);

    mqtt_event_t e = {.kind = MQTT_EVENT_BARCODE, .arg = 25, .time_ms = 1000, .left_count = 16, .right_count = 17, .arg2 = 2};
    assert(mqtt_encode_event(buf, sizeof buf, &e) == MQTT_LEN_EVENT && memcmp(buf, golden_event, MQTT_LEN_EVENT) == 0);
    mqtt_event_t de;
    assert(mqtt_decode_event(golden_event, MQTT_LEN_EVENT, &de) == MQTT_DECODE_OK && de.arg == 25 && de.time_ms == 1000);

    mqtt_mapping_t m = {.sequence = 7, .letters = {0, 1, 2, 3}};		/* A left, B right, C straight, D U-turn */
    m.letters[0] = 0; m.letters[1] = 1; m.letters[2] = 2; m.letters[3] = 3;
    assert(mqtt_encode_mapping(buf, sizeof buf, &m) == MQTT_LEN_MAPPING);
    assert(memcmp(buf, golden_mapping, MQTT_LEN_MAPPING) == 0);
    mqtt_mapping_t dm;
    assert(mqtt_decode_mapping(buf, MQTT_LEN_MAPPING, &dm) == MQTT_DECODE_OK && dm.letters[3] == 3 && dm.sequence == 7);

    mqtt_ack_t a = {.sequence = 7, .result = MQTT_ACK_OK}, da;
    assert(mqtt_encode_ack(buf, sizeof buf, &a) == MQTT_LEN_ACK && memcmp(buf, golden_ack, MQTT_LEN_ACK) == 0);
    assert(mqtt_decode_ack(golden_ack, MQTT_LEN_ACK, &da) == MQTT_DECODE_OK && da.result == MQTT_ACK_OK);

    uint8_t seq;
    assert(mqtt_encode_snapshot_request(buf, sizeof buf, 9) == MQTT_LEN_SNAPSHOT_REQUEST);
    assert(memcmp(buf, golden_request, MQTT_LEN_SNAPSHOT_REQUEST) == 0);
    assert(mqtt_decode_snapshot_request(golden_request, MQTT_LEN_SNAPSHOT_REQUEST, &seq) == MQTT_DECODE_OK && seq == 9);

    /* Snapshot reply shares the telemetry body. */
    assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_SNAPSHOT_REPLY, &t) == MQTT_LEN_TELEMETRY && (buf[0] & 0xF) == 3);
    assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_OK && ty == MQTT_TYPE_SNAPSHOT_REPLY);
}

static void test_boundaries_and_rejections(void)
{
    uint8_t buf[MQTT_FRAME_MAX];
    mqtt_telemetry_t t = sample();
    /* Maxima encode; one past each maximum is rejected. */
    t.mission_state = 15; t.motion_status = 3; t.motion_action = 63; t.hump_max = 127; t.front_mm = 8191;
    t.hump_valid = false; t.front_valid = false;
    t.barcode_letter = MQTT_NO_LETTER; t.barcode_action = 4;
    assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &t) == MQTT_LEN_TELEMETRY);
    mqtt_type_t ty; mqtt_telemetry_t d;
    assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_OK && d.front_mm == 8191u);
    mqtt_telemetry_t over;
    over = t; over.mission_state = 16; assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &over) == 0);
    over = t; over.line_status = 4; assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &over) == 0);
    over = t; over.motion_action = 64; assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &over) == 0);
    over = t; over.hump_max = 128; assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &over) == 0);
    over = t; over.front_mm = 8192; assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &over) == 0);
    over = t; over.barcode_action = 5; assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &over) == 0);
    over = t; over.barcode_letter = 26; assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &over) == 0);
    over = t; over.front_valid = true; assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &over) == 0);
    over = t; over.hump_valid = true; assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_TELEMETRY, &over) == 0);
    assert(mqtt_encode_telemetry(buf, MQTT_LEN_TELEMETRY - 1u, MQTT_TYPE_TELEMETRY, &t) == 0);	/* short buffer */
    assert(mqtt_encode_telemetry(buf, sizeof buf, MQTT_TYPE_ACK, &t) == 0);			/* wrong type */

    /* Decoder validation: length, version, type, reserved bits, ranges. */
    memcpy(buf, golden_telemetry, MQTT_LEN_TELEMETRY);
    assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY - 1u, &ty, &d) == MQTT_DECODE_LENGTH);
    assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY + 1u, &ty, &d) == MQTT_DECODE_LENGTH);
    assert(mqtt_decode_telemetry(buf, 0u, &ty, &d) == MQTT_DECODE_NULL);
    assert(mqtt_decode_telemetry(0, 5u, &ty, &d) == MQTT_DECODE_NULL);
    buf[0] = 0x11; assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_VERSION);	/* schema v1 */
    buf[0] = 0x2F; assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_TYPE);
    buf[0] = 0x20; assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_TYPE);
    buf[0] = 0x25; assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_LENGTH);	/* ack type, telemetry length */
    memcpy(buf, golden_telemetry, MQTT_LEN_TELEMETRY); buf[3] |= 0x80;
    assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_RESERVED);
    memcpy(buf, golden_telemetry, MQTT_LEN_TELEMETRY); buf[17] |= 0xC0;
    assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_RANGE);
    memcpy(buf, golden_telemetry, MQTT_LEN_TELEMETRY); buf[18] = 0x1A;		/* letter 26 */
    assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_RANGE);

    memcpy(buf, golden_telemetry, MQTT_LEN_TELEMETRY); buf[18] = 5u << 5;
    assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_RANGE);
    memcpy(buf, golden_telemetry, MQTT_LEN_TELEMETRY); buf[15] = 0xFFu;
    assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_RANGE);
    memcpy(buf, golden_telemetry, MQTT_LEN_TELEMETRY); buf[16] = 0xFFu; buf[17] = 0x3Fu;
    assert(mqtt_decode_telemetry(buf, MQTT_LEN_TELEMETRY, &ty, &d) == MQTT_DECODE_RANGE);

    /* Mapping: duplicates, out-of-range letters and reserved bits are rejected both ways. */
    mqtt_mapping_t m = {.sequence = 1, .letters = {0, 1, 2, 3}};
    m.letters[1] = 0; assert(mqtt_encode_mapping(buf, sizeof buf, &m) == 0);
    m.letters[1] = 26; assert(mqtt_encode_mapping(buf, sizeof buf, &m) == 0);
    m.letters[0] = 25; m.letters[1] = 24; m.letters[2] = 23; m.letters[3] = 22;	/* Z Y X W: maxima */
    assert(mqtt_encode_mapping(buf, sizeof buf, &m) == MQTT_LEN_MAPPING);
    mqtt_mapping_t dm; assert(mqtt_decode_mapping(buf, MQTT_LEN_MAPPING, &dm) == MQTT_DECODE_OK && dm.letters[0] == 25);
    buf[4] |= 0x10; assert(mqtt_decode_mapping(buf, MQTT_LEN_MAPPING, &dm) == MQTT_DECODE_RESERVED);
    memcpy(buf, (uint8_t[]){0x24, 0, 0x20, 0x00, 0x00}, MQTT_LEN_MAPPING);	/* letters 0,1,0,0: duplicates */
    assert(mqtt_decode_mapping(buf, MQTT_LEN_MAPPING, &dm) == MQTT_DECODE_RANGE);
    assert(mqtt_decode_mapping(buf, MQTT_LEN_MAPPING - 1u, &dm) == MQTT_DECODE_LENGTH);

    /* Event and ack ranges. */
    mqtt_event_t e = {.kind = 0}; assert(mqtt_encode_event(buf, sizeof buf, &e) == 0);
    e.kind = MQTT_EVENT_COUNT; assert(mqtt_encode_event(buf, sizeof buf, &e) == 0);
    memcpy(buf, golden_event, MQTT_LEN_EVENT); buf[1] = 0x11; mqtt_event_t de;
    assert(mqtt_decode_event(buf, MQTT_LEN_EVENT, &de) == MQTT_DECODE_RESERVED);
    buf[1] = 0x09; assert(mqtt_decode_event(buf, MQTT_LEN_EVENT, &de) == MQTT_DECODE_RANGE);
    mqtt_ack_t a = {.sequence = 1, .result = MQTT_ACK_MALFORMED + 1}; assert(mqtt_encode_ack(buf, sizeof buf, &a) == 0);
    memcpy(buf, golden_ack, MQTT_LEN_ACK); buf[2] = 4; mqtt_ack_t da;
    assert(mqtt_decode_ack(buf, MQTT_LEN_ACK, &da) == MQTT_DECODE_RANGE);
}

/* S1-U13: each owner byte retains every bit independently on the wire. */
static void test_fault_bytes(void)
{
    for (unsigned owner = 0u; owner < MQTT_FAULT_BYTES; ++owner)
    {
        for (unsigned bit = 0u; bit < 8u; ++bit)
        {
            mqtt_telemetry_t t = sample(), back;
            mqtt_type_t type;
            uint8_t frame[MQTT_LEN_TELEMETRY];
            memset(t.faults, 0, sizeof t.faults);
            t.faults[owner] = (uint8_t)(1u << bit);
            assert(mqtt_encode_telemetry(frame, sizeof frame,
                MQTT_TYPE_TELEMETRY, &t) == sizeof frame);
            assert(frame[4u + owner] == t.faults[owner]);
            assert(mqtt_decode_telemetry(frame, sizeof frame, &type, &back) == MQTT_DECODE_OK);
            assert(memcmp(t.faults, back.faults, sizeof t.faults) == 0);
        }
    }
    puts("S1-U13: PASS (six fault bytes, all 48 walking bits)");
}

/* Calibration must survive telemetry/snapshot transport without a hump result. */
static void test_imu_calibration(void)
{
    for (unsigned status = MQTT_IMU_UNKNOWN; status <= MQTT_IMU_CALIBRATED; ++status)
    {
        mqtt_telemetry_t t = sample(), back;
        mqtt_type_t type;
        uint8_t frame[MQTT_LEN_TELEMETRY];
        t.imu_calibration = (uint8_t)status;
        t.hump_count = 0u;
        t.hump_valid = false;
        t.front_mm = 0u;
        for (unsigned reply = 0u; reply <= 1u; ++reply)
        {
            mqtt_type_t expected = reply ? MQTT_TYPE_SNAPSHOT_REPLY : MQTT_TYPE_TELEMETRY;
            assert(mqtt_encode_telemetry(frame, sizeof frame, expected, &t) == sizeof frame);
            assert((frame[17] >> 6) == status);
            assert(mqtt_decode_telemetry(frame, sizeof frame, &type, &back) == MQTT_DECODE_OK);
            assert(type == expected && back.imu_calibration == status);
            assert(back.front_valid && back.front_mm == 0u && !back.hump_valid);
            frame[16] = 0xFFu;
            frame[17] |= 0x3Fu; /* Invalid front range, regardless of calibration. */
            assert(mqtt_decode_telemetry(frame, sizeof frame, &type, &back) == MQTT_DECODE_RANGE);
        }
    }
    mqtt_telemetry_t invalid = sample();
    uint8_t frame[MQTT_LEN_TELEMETRY];
    invalid.imu_calibration = 3u;
    assert(mqtt_encode_telemetry(frame, sizeof frame, MQTT_TYPE_TELEMETRY, &invalid) == 0u);
    puts("IMU calibration: PASS (unknown/not calibrated/calibrated, both frame types, independent range/hump validity)");
}

int main(void)
{
    uint8_t sequence, action;
    uint8_t direction[] = {0x27u, 255u, 1u};
    for (uint8_t a = 1u; a <= 4u; ++a)
    {
        direction[2] = a;
        assert(mqtt_decode_junction_test(direction, 3u, &sequence, &action) == MQTT_DECODE_OK);
        assert(sequence == 255u && action == a);
    }
    direction[2] = 0u;
    assert(mqtt_decode_junction_test(direction, 3u, &sequence, &action) == MQTT_DECODE_RANGE);
    direction[2] = 5u;
    assert(mqtt_decode_junction_test(direction, 3u, &sequence, &action) == MQTT_DECODE_RANGE);
    direction[2] = 9u;
    assert(mqtt_decode_junction_test(direction, 3u, &sequence, &action) == MQTT_DECODE_RESERVED);
    assert(mqtt_decode_junction_test(direction, 2u, &sequence, &action) == MQTT_DECODE_LENGTH);
    test_golden();
    test_imu_calibration();
    test_fault_bytes();
    test_boundaries_and_rejections();
    printf("mqtt_frame: PASS (telemetry %u bytes)\n", MQTT_LEN_TELEMETRY);
    return 0;
}
