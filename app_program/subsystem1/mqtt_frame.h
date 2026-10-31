/*
 *	mqtt_frame.h
 *	Binary MQTT frames, schema version 2 (docs/architecture.md, MQTT binary
 *	protocol). Little-endian,
 *	explicit masks and shifts, never a struct copy. Kernel-free: encode and
 *	decode are pure functions over byte buffers.
 *
 *	byte 0 of every frame: bits 7-4 version, bits 3-0 type.
 */

#ifndef MQTT_FRAME_H
#define MQTT_FRAME_H

#include <stdbool.h>
#include <stdint.h>

#define MQTT_FRAME_VERSION 2u

typedef enum
{
    MQTT_TYPE_TELEMETRY = 1,	/* robot -> PC, every 500 ms */
    MQTT_TYPE_EVENT = 2,	/* robot -> PC, one event record */
    MQTT_TYPE_SNAPSHOT_REPLY = 3,	/* robot -> PC, same body as telemetry */
    MQTT_TYPE_MAPPING = 4,	/* PC -> robot, four letters */
    MQTT_TYPE_ACK = 5,		/* robot -> PC, {sequence, result} */
    MQTT_TYPE_SNAPSHOT_REQUEST = 6,	/* PC -> robot */
    MQTT_TYPE_JUNCTION_TEST = 7, /* PC -> Mission, next-junction test direction */
    MQTT_TYPE_WHEEL_TRACE = 8 /* robot -> PC, best-effort Motion-release trace */
} mqtt_type_t;

/* Frame lengths in bytes (exact). */
#define MQTT_LEN_TELEMETRY 23u
#define MQTT_LEN_EVENT 11u
#define MQTT_LEN_MAPPING 5u
#define MQTT_LEN_JUNCTION_TEST 3u
#define MQTT_LEN_WHEEL_TRACE 15u
#define MQTT_LEN_ACK 3u
#define MQTT_LEN_SNAPSHOT_REQUEST 2u
#define MQTT_FRAME_MAX 23u

typedef enum
{
    MQTT_DECODE_OK,
    MQTT_DECODE_NULL,
    MQTT_DECODE_LENGTH,	/* wrong length for the type */
    MQTT_DECODE_VERSION,
    MQTT_DECODE_TYPE,
    MQTT_DECODE_RESERVED,	/* a reserved bit is set */
    MQTT_DECODE_RANGE	/* a field is outside its range (letter, state, code) */
} mqtt_decode_t;

#define MQTT_FAULT_BYTES 6u	/* rd_owner_t order: Motion, Line, Terrain, Obstacle, Mission, Network */
#define MQTT_NO_LETTER 31u	/* barcode letter field when nothing decoded yet */

typedef enum
{
    MQTT_IMU_UNKNOWN,
    MQTT_IMU_NOT_CALIBRATED,
    MQTT_IMU_CALIBRATED
} mqtt_imu_calibration_t;

typedef struct
{
    uint8_t sequence;
    uint8_t mission_state;	/* 0..15 */
    uint8_t motion_status;	/* 0..3: IDLE, RUNNING, DONE, FAILED */
    uint8_t motion_action;	/* 0..63 */
    uint8_t line_status;	/* 0..3: IrLine (UNKNOWN, VISIBLE, LOST, BROAD_BLACK) */
    bool mapped;		/* four barcode letters installed */
    uint8_t faults[MQTT_FAULT_BYTES];
    uint16_t left_count, right_count;	/* wrapped encoder counts */
    uint8_t hump_count;
    uint8_t hump_max;		/* 7-bit result code */
    bool hump_valid;
    uint8_t imu_calibration : 2; /* mqtt_imu_calibration_t; 3 invalid */
    uint16_t front_mm;		/* 0..8190 */
    bool front_valid;
    uint8_t barcode_letter;	/* 0..25 or MQTT_NO_LETTER */
    uint8_t barcode_action;	/* 0..7 (barcode_action_t) */
    uint16_t obstacle_width_mm;	/* latest profile; 0 none */
    uint16_t obstacle_depth_mm;	/* latest bypass; 0 none */
} mqtt_telemetry_t;

typedef enum
{
    MQTT_EVENT_BARCODE = 1,	/* arg: letter (0..25), arg2: action */
    MQTT_EVENT_HUMP,		/* arg: 7-bit height code */
    MQTT_EVENT_OBSTACLE,	/* arg2: depth mm along the obstacle */
    MQTT_EVENT_TURN,		/* arg: action */
    MQTT_EVENT_COUNT
} mqtt_event_kind_t;

typedef struct
{
    uint8_t kind;		/* mqtt_event_kind_t, 1..4 */
    uint8_t arg;
    uint16_t time_ms;		/* low 16 bits of kernel ms */
    uint16_t left_count, right_count;
    uint16_t arg2;
} mqtt_event_t;

typedef struct
{
    uint8_t sequence;
    uint8_t letters[4];		/* A..Z as 0..25: left, right, straight, U-turn */
} mqtt_mapping_t;

typedef enum
{
    MQTT_ACK_OK,
    MQTT_ACK_BUSY,		/* occupied hand-off or request refused in current Mission state */
    MQTT_ACK_INVALID,	/* invalid mapping letters or junction-test action */
    MQTT_ACK_MALFORMED
} mqtt_ack_result_t;

typedef struct
{
    uint8_t sequence;
    uint8_t result;		/* mqtt_ack_result_t */
} mqtt_ack_t;

/* Encoders write exactly the type's length and return it, or 0 when a field
   is out of range (nothing written beyond `capacity` either way). */
unsigned mqtt_encode_telemetry(uint8_t *out, unsigned capacity, mqtt_type_t type, const mqtt_telemetry_t *t);
unsigned mqtt_encode_event(uint8_t *out, unsigned capacity, const mqtt_event_t *e);
unsigned mqtt_encode_mapping(uint8_t *out, unsigned capacity, const mqtt_mapping_t *m);
unsigned mqtt_encode_ack(uint8_t *out, unsigned capacity, const mqtt_ack_t *a);
unsigned mqtt_encode_snapshot_request(uint8_t *out, unsigned capacity, uint8_t sequence);

/* Local encoder input only; persistent traces store encoded bytes. */
typedef struct
{
    uint32_t sampled_ms;
    float target_cps[2], measured_cps[2], duty[2];
    bool inhibited, output_ok;
} mqtt_wheel_trace_t;

unsigned mqtt_encode_wheel_trace(uint8_t *out, unsigned capacity,
    const mqtt_wheel_trace_t *trace);

/* Header check: version, type and the exact length for that type. */
mqtt_decode_t mqtt_frame_type(const uint8_t *frame, unsigned length, mqtt_type_t *type);

mqtt_decode_t mqtt_decode_telemetry(const uint8_t *frame, unsigned length, mqtt_type_t *type, mqtt_telemetry_t *t);
mqtt_decode_t mqtt_decode_event(const uint8_t *frame, unsigned length, mqtt_event_t *e);
mqtt_decode_t mqtt_decode_junction_test(const uint8_t *frame, unsigned length,
    uint8_t *sequence, uint8_t *action);

mqtt_decode_t mqtt_decode_mapping(const uint8_t *frame, unsigned length, mqtt_mapping_t *m);
mqtt_decode_t mqtt_decode_ack(const uint8_t *frame, unsigned length, mqtt_ack_t *a);
mqtt_decode_t mqtt_decode_snapshot_request(const uint8_t *frame, unsigned length, uint8_t *sequence);

#endif /* MQTT_FRAME_H */
