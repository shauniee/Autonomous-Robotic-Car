/* Comms public interface: wire frames, TX/RX bridge, event log and task API. */
#ifndef COMMS_H
#define COMMS_H

#include <stdbool.h>
#include <stdint.h>
#include "../common/robot_config.h"

/*
 *  Binary MQTT frames, schema version 3 (docs/architecture.md, MQTT binary
 *	protocol). Little-endian,
 *	explicit masks and shifts, never a struct copy. Kernel-free: encode and
 *	decode are pure functions over byte buffers.
 *
 *	byte 0 of every frame: bits 7-4 version, bits 3-0 type.
 */

#define MQTT_FRAME_VERSION 3u

typedef enum
{
    MQTT_TYPE_TELEMETRY = 1,	/* robot -> PC, every 500 ms */
    MQTT_TYPE_EVENT = 2,	/* robot -> PC, one event record */
    MQTT_TYPE_SNAPSHOT_REPLY = 3,	/* robot -> PC, same body as telemetry */
    MQTT_TYPE_MAPPING = 4,	/* PC -> robot, four letters */
    MQTT_TYPE_ACK = 5,		/* robot -> PC, {sequence, result} */
    MQTT_TYPE_SNAPSHOT_REQUEST = 6,	/* PC -> robot */
    MQTT_TYPE_JUNCTION_TEST = 7, /* PC -> Mission, next-junction test direction */
    MQTT_TYPE_WHEEL_TRACE = 8, /* robot -> PC, best-effort Motion-release trace */
    MQTT_TYPE_LINE_TRACE = 9, /* robot -> PC, best-effort Mission line decision trace */
    MQTT_TYPE_BARCODE_TRACE = 10, /* robot -> PC, best-effort raw GP16 run widths */
    MQTT_TYPE_TERRAIN_TRACE = 11 /* robot -> PC, best-effort hump detector state */
} mqtt_type_t;

/* Frame lengths in bytes (exact). */
#define MQTT_OBSTACLE_MAX_MM 1023u	/* 10 bits each on the wire */
#define MQTT_LEN_TELEMETRY 18u
#define MQTT_LEN_EVENT 11u
#define MQTT_LEN_MAPPING 5u
#define MQTT_LEN_JUNCTION_TEST 3u
#define MQTT_LEN_WHEEL_TRACE 15u
#define MQTT_LEN_LINE_TRACE 17u
#define MQTT_LEN_BARCODE_TRACE 18u
#define MQTT_BARCODE_TRACE_RUNS 8u
#define MQTT_LEN_TERRAIN_TRACE 14u
#define MQTT_LEN_ACK 3u
#define MQTT_LEN_SNAPSHOT_REQUEST 2u
#define MQTT_FRAME_MAX 18u

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

/* In-RAM copy, one byte per owner in rd_owner_t order: Motion, Line, Terrain,
   Obstacle, Mission, Network. The wire carries them as one 12-bit word. */
#define MQTT_FAULT_BYTES 6u
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
    uint16_t obstacle_width_mm;	/* latest profile; 0 none; saturates at MQTT_OBSTACLE_MAX_MM */
    /* Room on the side chosen; 0 none; saturates at MQTT_OBSTACLE_MAX_MM. */
    uint16_t obstacle_clearance_mm;
} mqtt_telemetry_t;

typedef enum
{
    MQTT_EVENT_BARCODE = 1,	/* arg: letter (0..25), arg2: action */
    MQTT_EVENT_HUMP,		/* arg: 7-bit height code */
    /* arg: 1 passing left, 0 right; arg2: clearance on that side, mm. */
    MQTT_EVENT_OBSTACLE,
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

/* Line trace flag bits (byte 5). */
#define MQTT_LINE_LEFT_BLACK	0x01u
#define MQTT_LINE_RIGHT_BLACK	0x02u
#define MQTT_LINE_IN_BROAD	0x04u
#define MQTT_LINE_JUNCTION_OPEN	0x08u
#define MQTT_LINE_JUNCTION_QUALIFIED	0x10u
#define MQTT_LINE_ENTRY_CURVED	0x20u
#define MQTT_LINE_JUNCTION_LOCKED	0x40u
#define MQTT_LINE_BARCODE_LEVEL	0x80u

/* Local encoder input, one per fresh IR acquisition Mission acted on. */
typedef struct
{
    uint32_t sampled_ms;	/* IR acquisition time */
    uint8_t flags;		/* MQTT_LINE_* */
    uint8_t mission_state;	/* 0..15 */
    uint8_t phase;		/* junction phase, 0..7 */
    uint16_t wheel_count[2];	/* low 16 bits of the shared encoder counts */
    int8_t steer_q;		/* steering law output, Q7 */
    int8_t law_q;		/* last side times the integral held at the last touch transition, Q7 */
    int8_t command_steer_q;	/* FOLLOW steering sent to Motion, else 0 */
    uint8_t command_mode;	/* motion_mode_t, 0..3 */
    uint16_t command_speed;	/* mm/s sent to Motion, 0..511 */
    uint8_t onset_skew_mm;	/* both black: mm between the black onsets, 254 max; else 255 */
    uint8_t barcode_edges;	/* running GP16 edge count, 0..31, wraps */
} mqtt_line_trace_t;

/* Terrain trace for the dashboard's IMU graphs. Tilt is in quarter degrees;
   acceleration axes saturate at +/-2032 mg; height is the current estimate and
   rise the peak this encounter; travel wraps at 65536 mm. */
typedef struct
{
    uint16_t time_ms;
    int8_t pitch_q2;
    uint8_t state_flags;
    uint8_t rise_mm;
    uint8_t accel_10mg;
    int8_t roll_q2;
    int8_t accel_16mg[3];
    uint8_t height_mm;
    uint16_t travel_mm;
} mqtt_terrain_trace_t;

unsigned mqtt_encode_terrain_trace(uint8_t *p_out, unsigned capacity,
    const mqtt_terrain_trace_t *p_trace);

/* Barcode trace: 8 raw GP16 runs, each bit 15 black level, bits 14..0 width in counts. */
unsigned mqtt_encode_barcode_trace(uint8_t *out, unsigned capacity, uint8_t sequence,
    const uint16_t runs[MQTT_BARCODE_TRACE_RUNS]);

unsigned mqtt_encode_line_trace(uint8_t *out, unsigned capacity,
    const mqtt_line_trace_t *trace);

/* Header check: version, type and the exact length for that type. */
mqtt_decode_t mqtt_frame_type(const uint8_t *frame, unsigned length, mqtt_type_t *type);

mqtt_decode_t mqtt_decode_telemetry(const uint8_t *frame, unsigned length, mqtt_type_t *type, mqtt_telemetry_t *t);
mqtt_decode_t mqtt_decode_event(const uint8_t *frame, unsigned length, mqtt_event_t *e);
mqtt_decode_t mqtt_decode_junction_test(const uint8_t *frame, unsigned length,
    uint8_t *sequence, uint8_t *action);

mqtt_decode_t mqtt_decode_mapping(const uint8_t *frame, unsigned length, mqtt_mapping_t *m);
mqtt_decode_t mqtt_decode_ack(const uint8_t *frame, unsigned length, mqtt_ack_t *a);
mqtt_decode_t mqtt_decode_snapshot_request(const uint8_t *frame, unsigned length, uint8_t *sequence);

/*
 *	The only application header the MQTT transport (lib/liblwip) includes.
 *	Frames cross as opaque bytes; the transport knows nothing of their layout
 *	(see comms.h). Two fixed single-producer / single-consumer rings:
 *	  outgoing: I/O (core 0) posts, transport (core 0) takes
 *	  incoming: transport (core 0) puts, I/O (core 0) receives
 *	Wi-Fi or broker loss sets only the Network fault byte; it never stops motion.
 */

/* Largest frame, MQTT_FRAME_MAX (telemetry/snapshot). */
#define COMMS_FRAME_MAX 18u
#define COMMS_OUTGOING_DEPTH 4u
#define COMMS_INCOMING_DEPTH 2u

/* I/O side (core 0). post: false while offline or when the ring is full. */
bool comms_bridge_post(const uint8_t *frame, unsigned length);
/* Returns the length written to out, or 0 when nothing is waiting. */
unsigned comms_bridge_receive(uint8_t *out, unsigned capacity);

/* Transport side (core 0). take: length written, or 0 when nothing is waiting. */
unsigned comms_bridge_take(uint8_t *out, unsigned capacity);
/* Offline frames, oversized frames and frames arriving to a full ring drop. */
void comms_bridge_put(const uint8_t *frame, unsigned length);
/* Link state from transport; updates Network faults and discards queued TX/RX
   on loss/reconnection. Never changes motion or Mission state. */
void comms_bridge_link(bool wifi_up, bool broker_up);
/* Boot is disconnected. Mission uses this only as a local-start prerequisite. */
bool comms_bridge_ready(void);

/*
 *	Kernel-free Reporting logic: the event-log ring and telemetry clamping.
 *	comms.c supplies the sources, mapping hand-off and the
 *	transport; nothing here touches the kernel.
 */

/* ---- Event log: fixed ring of packed events ------------------------------
   Only the persistent RAM entries are packed; mqtt_event_t is the decoded
   API value and the MQTT wire frame remains the unchanged 11-byte schema v2.

   kind 1..4 needs 3 bits; arg 0..127 needs 7 bits (height code is
   the widest arg). The resulting 10-bit word has six unused high bits.
   time, encoder counts and arg2 retain their full 16-bit ranges.
   For obstacle events, arg2 carries depth 0..65535 mm, so it cannot be
   narrowed to the three bits sufficient for a barcode action.

   Only comms_log_push/pop access this storage through internal pack/unpack
   accessors. Invalid kind/arg values are rejected, never silently truncated.
   sizeof(comms_packed_event_t) is 10 bytes on the target: the existing storage
   was already 10 bytes, so this change does not claim savings per event. */
typedef struct
{
    uint16_t kind_arg;       /* bits 0..2 kind, 3..9 arg; bits 10..15 zero */
    uint16_t time_ms;
    uint16_t left_count;
    uint16_t right_count;
    uint16_t arg2;
} comms_packed_event_t;

/* Capacity 16: head 0..15 uses 4 bits; count 0..16 uses 5 bits.
   The combined 9 bits reside in one 16-bit storage unit. A push into a
   full ring overwrites the oldest event; the newest events are retained.
   The ring is externally synchronised by its caller (comms.c uses a
   bounded cross-core mutex). */
typedef struct
{
    comms_packed_event_t events[COMMS_EVENT_CAPACITY];
    uint16_t indices;       /* bits 0..3 head, 4..8 count; bits 9..15 zero */
} comms_event_log_t;

_Static_assert(COMMS_EVENT_CAPACITY == 16u, "event-log index packing assumes 16 slots");
_Static_assert(sizeof(comms_packed_event_t) == 10u, "event packing must occupy 10 bytes");
_Static_assert(sizeof(comms_event_log_t) == 162u,
    "event ring layout changed unexpectedly");

void comms_log_init(comms_event_log_t *log);
bool comms_log_push(comms_event_log_t *log, const mqtt_event_t *event);
bool comms_log_pop(comms_event_log_t *log, mqtt_event_t *event); /* oldest first */
typedef bool (*comms_post_fn)(const uint8_t *frame, unsigned length);
/* Retains the oldest event if posting fails. Caller synchronizes the ring. */
bool comms_log_post_oldest(comms_event_log_t *log, comms_post_fn post);
unsigned comms_log_count(const comms_event_log_t *log);

/* Clamps a gathered telemetry record to its field widths in place, so the
   encoder never rejects it. */
void comms_clamp(mqtt_telemetry_t *t);

/* Shaun: communications. Reporting task: binary telemetry, event log, mapping command. */
ER comms_init(void);
void comms_step(uint32_t now_ms);

/* Log one event with the current time and encoder position. Mission (core 1) or Terrain (core 0);
   insertion waits at most RD_SNAPSHOT_LOCK_MS (2 ms) and a failed lock drops
   the event; a full ring overwrites the oldest event (see comms.h).
   kind: mqtt_event_kind_t; arg and arg2 as documented in MQTT_PROTOCOL.md. */
void comms_event(mqtt_event_kind_t kind, uint8_t arg, uint16_t arg2);

#endif /* COMMS_H */
