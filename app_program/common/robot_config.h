/*
 *	robot_config.h
 *	Every tunable constant in one header, as #define. Relations between
 *	constants are _Static_assert, so an inconsistent edit fails the build.
 *	No runtime configuration, parsing or validation structs.
 *
 *	"PROVISIONAL (Dnn)" marks a value that the pending decision Dnn in
 *	docs/PROJECT_SPECS.md will set; change it there first, then here.
 *	Register addresses, bit fields and device register maps stay in their
 *	drivers: they are facts of the chip, not tuning.
 */

#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

/* --- Tasks ---------------------------------------------------------- */
#define TASK_STACK_SIZE		2048	/* bytes; tm_printf needs ~1 KB */
#define S3_PRI_LINE		12
#define S2_PRI_MOTION		13
#define FOLLOW_PRIORITY		14
#define S4_PRI_TERRAIN		16

/* --- S1 communications --------------------------------------------- */
#define S1_REQUEST_MAX			64u
#define S1_STATUS_MAX			256u
#define S1_HEARTBEAT_PERIOD_MS		1000u
#define S1_TELEMETRY_PERIOD_MS		500u
#define S1_TELEMETRY_PAYLOAD_MAX	768u	/* JSON; replaced by #39 */

/* --- S2 motion ------------------------------------------------------ */
#define MOTION_MAX_DUTY		.35f	/* PROVISIONAL (D07) */
#define MOTION_EXPIRY_MS	60u	/* PROVISIONAL (D07): sample age limit */
#define MOTOR_WRAP		999u	/* 20 kHz at 125 MHz with divider below */
#define MOTOR_DIV_8_4		100u
#define LEFT_FORWARD_A		1	/* PROVISIONAL (D03): polarity */
#define RIGHT_FORWARD_A		1	/* PROVISIONAL (D03): polarity */

/* Encoder: one wrapping count per wheel (ARCHITECTURE, Encoder contract). */
#define ENC_COUNT_BITS		16u	/* PROVISIONAL (D03): width from counts/rev and longest segment */
#define LEFT_ENCODER_SIGN	1	/* PROVISIONAL (D03): polarity */
#define RIGHT_ENCODER_SIGN	1	/* PROVISIONAL (D03): polarity */
#define ENC_LEFT_A_PIN		28u
#define ENC_LEFT_B_PIN		7u
#define ENC_RIGHT_A_PIN		1u
#define ENC_RIGHT_B_PIN		0u

/* Motion geometry and control. UNMEASURED PLACEHOLDERS until #32 measures
   them (D03 geometry, D07 gains/thresholds, D08 stationary threshold). */
#define ENC_COUNTS_PER_REV	360.0f	/* PROVISIONAL (D03): decoded counts per wheel rev */
#define WHEEL_DIAMETER_MM	65.0f	/* PROVISIONAL (D03) */
#define TRACK_WIDTH_MM		130.0f	/* PROVISIONAL (D03) */
#define MOTION_SPEED_MAX_MMPS	300.0f	/* PROVISIONAL (D07): commanded speed limit */
#define MOTION_KP		0.0003f	/* PROVISIONAL (D07): duty per count/s */
#define MOTION_KI		0.0010f	/* PROVISIONAL (D07) */
#define MOTION_KD		0.0000f	/* PROVISIONAL (D07) */
#define MOTION_DIST_TOL_MM	3.0f	/* PROVISIONAL (D07) */
#define MOTION_ANGLE_TOL_DEG	2.0f	/* PROVISIONAL (D07) */
#define MOTION_APPROACH_GAIN	4.0f	/* 1/s: speed = gain * remaining distance */
#define MOTION_STATIONARY_CPS	5.0f	/* PROVISIONAL (D08): below this a wheel is stationary */
#define MOTION_STALL_MS		400u	/* PROVISIONAL (D07) */
#define MOTION_ACTION_TIMEOUT_MS 8000u	/* PROVISIONAL (D07) */
#define MOTION_PERIOD_MS	20u

/* --- S3 line ------------------------------------------------------- */
#define IR_ADC_MAX		4095u
#define IR_SAMPLE_MS		20u
#define IR_MAX_GAP_MS		60u
#define IR_CAPTURE_MS		500u
#define IR_CAPTURE_MIN_SAMPLES	25u
#define IR_MIN_CONTRAST		300u	/* PROVISIONAL (D04) */
#define IR_MAX_CORRECTION	0.10f	/* PROVISIONAL (D04) */
#define IR_KP			0.05f	/* PROVISIONAL (D07) */
#define IR_WHITE_ENTER		0.15f	/* PROVISIONAL (D04) */
#define IR_WHITE_EXIT		0.25f	/* PROVISIONAL (D04) */
#define IR_BROAD_BLACK		0.85f	/* PROVISIONAL (D04) */
#define IR_LEFT_PIN		26u
#define IR_RIGHT_PIN		27u
#define ADC_BUDGET_US		2000u
#define ADC_POLL_LIMIT		4096u
#define ADC_AVERAGE_COUNT	8u

#define INPUT_PERIOD_MS		10u
#define REPORT_PERIOD_MS	500u
#define FOLLOW_PERIOD_MS	10u
#define RECOVERY_MS		500u	/* PROVISIONAL (D07) */
#define BASE_DUTY		0.25f	/* PROVISIONAL (D07) */
#define BROAD_DUTY		0.20f	/* PROVISIONAL (D07) */
#define MAX_STEER		0.40f	/* keeps both wheels within MOTION_MAX_DUTY */

/* --- S3 barcode ---------------------------------------------------- */
#define BARCODE_GPIO		16u
#define BARCODE_EVENT_CAPACITY	8u
#define BARCODE_EDGE_CAPACITY	64u	/* power of two: indices wrap unsigned */
#define BARCODE_SERVICE_BUDGET	16u

/* --- S4 terrain ---------------------------------------------------- */
#define TERRAIN_PERIOD_MS	20u
#define GY511_TIMEOUT_US	5000u
#define ACCEL_MG_PER_LSB	0.001f	/* 1 mg/digit at +/- 2 g */
#define LPF_ALPHA		0.20f	/* IIR filter weight; PROVISIONAL (D06) */
#define PITCH_CLIMB_THRESH_DEG	3.5f	/* PROVISIONAL (D06) */
#define PITCH_DESC_THRESH_DEG	-3.5f	/* PROVISIONAL (D06) */
#define IMPACT_THRESHOLD_G	2.5f	/* PROVISIONAL (D06) */
#define HUMP_RESULT_BITS	7u	/* reported height: 7 bits, all-ones is the invalid code */
#define HUMP_MM_PER_COUNT	1u	/* PROVISIONAL (D06): scale of the reported 7-bit result */
#define HUMP_MIN_HEIGHT_MM	5u	/* PROVISIONAL (D06): smaller rises are not humps */
#define HUMP_FLAT_SAMPLES	10u	/* PROVISIONAL (D06): releases at crest before it counts as flat ground */
#define HUMP_EVENT_CAPACITY	8u	/* per-hump events kept; the count keeps running */
#define HUMP_CALIB_FACTOR	1.15f	/* PROVISIONAL (D06) */

/* --- S5 obstacle --------------------------------------------------- */
#define ECHO_PIN		2	/* PROVISIONAL (D05): <= 3.3 V before connecting */
#define TRIGGER_PIN		3

/* --- Shared inputs and outputs ------------------------------------- */
#define BUTTON_GP20		20u
#define BUTTON_GP21		21u
#define BUTTON_DEBOUNCE_MS	30u
#define BUZZER_PIN		22u
#define BUZZER_SLICE		3u
#define BUZZER_DIVIDER		1000u
#define BUZZER_WRAP		999u
#define BUZZER_LEVEL		500u
#define CHIRP_MS		100u
#define MAX_CHIRPS		3u

/* --- Relations ----------------------------------------------------- */
_Static_assert(MOTION_MAX_DUTY > 0.0f && MOTION_MAX_DUTY <= 1.0f,
	"motion duty limit must be a fraction");
_Static_assert(BASE_DUTY * (1.0f + MAX_STEER) <= MOTION_MAX_DUTY,
	"line-follow duty plus steering must fit the motion duty limit");
_Static_assert(BROAD_DUTY <= MOTION_MAX_DUTY, "broad-black duty must stay within the motion duty limit");
_Static_assert(MOTION_EXPIRY_MS > 2u * FOLLOW_PERIOD_MS,
	"motion sample age limit must span at least two follow releases");
_Static_assert(IR_MAX_GAP_MS > IR_SAMPLE_MS, "IR gap limit below sample period");
_Static_assert(IR_CAPTURE_MS / IR_SAMPLE_MS >= IR_CAPTURE_MIN_SAMPLES,
	"IR capture window cannot reach its minimum sample count");
_Static_assert(IR_MIN_CONTRAST < IR_ADC_MAX, "IR contrast exceeds ADC range");
_Static_assert(IR_WHITE_ENTER < IR_WHITE_EXIT && IR_WHITE_EXIT < IR_BROAD_BLACK,
	"IR classification thresholds must be ordered");
_Static_assert(ADC_AVERAGE_COUNT >= 1u && ADC_AVERAGE_COUNT <= 16u,
	"ADC average count out of range");
_Static_assert(IR_SAMPLE_MS >= INPUT_PERIOD_MS, "IR sampled faster than the input task runs");
_Static_assert((BARCODE_EDGE_CAPACITY & (BARCODE_EDGE_CAPACITY - 1u)) == 0u,
	"barcode edge ring capacity must be a power of two");
_Static_assert(BARCODE_EVENT_CAPACITY >= 1u, "barcode event queue empty");
_Static_assert(MOTOR_WRAP == 999u && MOTOR_DIV_8_4 == 100u,
	"PWM wrap and divider define the 20 kHz motor frequency");
_Static_assert(ENC_COUNT_BITS >= 8u && ENC_COUNT_BITS <= 32u, "encoder count width out of range");
_Static_assert((LEFT_ENCODER_SIGN == 1 || LEFT_ENCODER_SIGN == -1) &&
	(RIGHT_ENCODER_SIGN == 1 || RIGHT_ENCODER_SIGN == -1), "encoder sign must be +1 or -1");
_Static_assert(MOTION_SPEED_MAX_MMPS > 0.0f && WHEEL_DIAMETER_MM > 0.0f &&
	TRACK_WIDTH_MM > 0.0f && ENC_COUNTS_PER_REV > 0.0f, "motion geometry must be positive");
_Static_assert(MOTION_STALL_MS < MOTION_ACTION_TIMEOUT_MS, "stall limit must precede the action timeout");
_Static_assert(S3_PRI_LINE != S2_PRI_MOTION && S2_PRI_MOTION != FOLLOW_PRIORITY &&
	S3_PRI_LINE != FOLLOW_PRIORITY, "task priorities must be distinct");
_Static_assert(PITCH_DESC_THRESH_DEG < 0.0f && PITCH_CLIMB_THRESH_DEG > 0.0f,
	"pitch thresholds must straddle zero");
_Static_assert(S1_TELEMETRY_PAYLOAD_MAX <= S1_STATUS_MAX * 4u,
	"telemetry payload bound out of proportion to status buffer");

#endif /* ROBOT_CONFIG_H */
