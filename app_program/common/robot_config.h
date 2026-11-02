/*
 *	robot_config.h
 *	Every tunable constant in one header, as #define. Relations between
 *	constants are _Static_assert, so an inconsistent edit fails the build.
 *	No runtime configuration, parsing or validation structs.
 *
 *	"PROVISIONAL (Dnn)" marks a value that the pending decision Dnn in
 *	docs/project-specs.md will set; change it there first, then here.
 *	Register addresses, bit fields and device register maps stay in their
 *	drivers: they are facts of the chip, not tuning.
 */

#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

/* Subsystem headers stay kernel-free so host tests and pure code can include
   them; the kernel's size_t clashes with the C library's. Same type as the
   kernel's ER (tk/typedef.h), which C11 allows to be repeated. */
typedef int ER;

/* Bench-only build (make NOGUARDS=1): automatic guards off (Safety faults,
   start blocking, motor-stall brake, line-loss stop), GP21 still stops. */
#ifndef APP_NO_GUARDS
#define APP_NO_GUARDS 0
#endif

/* --- Tasks: provisional existing defaults, physical acceptance D13 ---- */
#define TASK_STACK_SIZE		2048	/* bytes; tm_printf needs ~1 KB */
#define MOTION_PRI		13
#define FOLLOW_PRIORITY		14
#define IO_PRIORITY 16 /* provisional existing Terrain priority; D13 */
#define IO_PERIOD_MS 10u /* existing calibration cadence; D13 */

/* --- S1 communications --------------------------------------------- */
#define COMMS_TELEMETRY_PERIOD_MS	500u
#define COMMS_REPORT_PERIOD_MS	50u	/* Reporting release: requests and events are served at this rate */
#define COMMS_EVENT_CAPACITY	16u	/* event-log ring; oldest overwritten, drops counted */
#define COMMS_REQUESTS_PER_RELEASE 2u /* bounded by incoming bridge depth */
#define COMMS_EVENTS_PER_RELEASE	2u

/* --- S2 motion ------------------------------------------------------ */
#define MOTION_MAX_DUTY		.80f	/* PROVISIONAL (D07): retained duty cap; verify motor/driver rating */
#define MOTION_TARGET_MAX_MM 2000.0f /* D07: bounded by longest Mission leg; hardware validation pending */
#define MOTION_EXPIRY_MS	60u	/* PROVISIONAL (D07): sample age limit */
#define MOTOR_WRAP		999u	/* 20 kHz at 125 MHz with divider below */
#define MOTOR_DIV_8_4		100u
#define LEFT_FORWARD_A		1	/* PROVISIONAL (D03): polarity */
#define RIGHT_FORWARD_A		1	/* PROVISIONAL (D03): polarity */

/* Encoder: one wrapping count per wheel (ARCHITECTURE, Encoder contract). */
#define ENC_COUNT_BITS		16u
#define ENC_COUNT_MASK		(ENC_COUNT_BITS >= 32u ? 0xFFFFFFFFu : ((1u << (ENC_COUNT_BITS % 32u)) - 1u))	/* PROVISIONAL (D03): width from counts/rev and longest segment */
#define LEFT_ENCODER_SIGN	-1	/* MEASURED 2026-10-28: counts fell while the rewired left wheel drove forward */
#define RIGHT_ENCODER_SIGN	1	/* PROVISIONAL (D03): polarity */
/* An encoder fault clears after this many valid counts (both wheels) with
   no new invalid transition: 64 counts are 16 complete quadrature cycles,
   enough to show both channels switching cleanly. This is a provisional
   electrical-progress threshold, not a calibrated wheel-travel distance. */
#define ENC_FAULT_CLEAR_COUNTS	64u
#define ENC_LEFT_A_PIN		28u
#define ENC_LEFT_B_PIN		7u
#define ENC_RIGHT_A_PIN		1u
#define ENC_RIGHT_B_PIN		0u

/* Motion geometry and control. UNMEASURED PLACEHOLDERS until #32 measures
   them (D03 geometry, D07 gains/thresholds, D08 stationary threshold).

   WHEEL SPEED TUNING: use per-release WHEEL traces, not only the laptop's
   500 ms count averages. Verify encoder signs/scale, then compare each mixed
   target with measured speed and serviced duty under load. Tune KS/KF from
   the plant, KP for response, then KI for persistent error; leave KD zero
   initially. Saturated duty needs diagnosis before increasing gains/cap.
   Keep IR classification and steering settings fixed during these trials.
   Procedure: app_program/motion/README.md, Wheel-speed tuning. */
#define ENC_COUNTS_PER_MM 11.649f /* INT-66: mean of both wheels over 500 mm */
#define TRACK_WIDTH_MM		120.0f	/* User-measured wheel centres; specs D03; effective turning calibration pending */
#define MOTION_SPEED_MAX_MMPS	500.0f	/* PROVISIONAL (D07): speed limit; the 9-bit command field holds 511 */
#define MOTION_KS		0.05f	/* PROVISIONAL (D07): INT-100 raised fit ~0 plus ~5 points measured floor load at 200 mm/s */
#define MOTION_KF		0.0001f	/* PROVISIONAL (D07): INT-100 raised-wheel fit 60-200 mm/s, left 1.05e-4, right 0.95e-4 */
#define MOTION_KP		0.0001f	/* PROVISIONAL (D07): duty per count/s; INT-101 0.0003 oscillated below 60 mm/s */
#define MOTION_KI		0.0010f	/* PROVISIONAL (D07) */
#define MOTION_KD		0.0000f	/* PROVISIONAL (D07) */
#define MOTION_DIST_TOL_MM	3.0f	/* PROVISIONAL (D07) */
#define MOTION_ANGLE_TOL_DEG	2.0f	/* PROVISIONAL (D07) */
#define MOTION_APPROACH_GAIN	4.0f	/* 1/s: speed = gain * remaining distance */
#define MOTION_STATIONARY_CPS	5.0f	/* PROVISIONAL (D08): below this a wheel is stationary */
#define MOTION_STALL_MS		400u	/* PROVISIONAL (D07) */
#define MOTION_ACTION_TIMEOUT_MS 12000u /* D07 trial: corrected turn distances */
#define MOTION_PERIOD_MS	20u

/* --- S3 line ------------------------------------------------------- */
#define IR_SAMPLE_MS		10u	/* INT-115: 10 ms reads were the first change that helped at 100 mm/s */
#define IR_MAX_GAP_MS		60u
#define IR_LEFT_PIN		26u
#define IR_RIGHT_PIN		27u


#define SAFETY_STALE_MS		60u	/* PROVISIONAL (D07): required-producer age limit */
#define SAFETY_BOOT_GRACE_MS	1500u	/* producers may be silent this long after boot */
#define REPORT_PERIOD_MS	500u
#define FOLLOW_PERIOD_MS	10u
#ifndef MISSION_RECOVERY_RELEASED
#define MISSION_RECOVERY_RELEASED 1
#endif
#ifndef MOTION_STALL_GUARD_ENABLED
#define MOTION_STALL_GUARD_ENABLED 0 /* user-requested trial configuration */
#endif
#define RECOVERY_MS 30000u
#define RECOVERY_TRAVEL_MM 1200u /* PROPOSAL D07: wheel excursion bound */
#define RECOVERY_SPEED_MMPS 36u /* D07: preserves pre-calibration count speed */
#define RECOVERY_TARGET_DEG 70 /* user: sweep each side of the stopped pose */
#define RECOVERY_TARGET_TOL_DEG 5
#define RECOVERY_OVERSHOOT_DEG 20 /* PROPOSAL D07/D08: coast before reversal */
#define RECOVERY_ALIGN_ERROR_Q 25 /* bypass original-line alignment only */
#define MISSION_CONFIRM_SAMPLES 3u
#define MOTION_FOLLOW_SLEW_MMPS2 250u /* PROVISIONAL D07: common FOLLOW base ramp */
#define MOTION_FOLLOW_STEER_QPS 900u /* PROVISIONAL D07: applied steering slew */
#define MISSION_TRACK_SPEED_MMPS 100.0f /* user hard guardrail: fixed course speed, not overridable */
/* White reported as black with LOW polarity; use HIGH for black. */
#define IR_BLACK_LEVEL 1u

/* --- Mission (S3). UNMEASURED PLACEHOLDERS until #59/#61 (D01, D02, D07) ----- */
/* Specs: requested hump speed is half normal tracking speed. */
#define MISSION_HUMP_SPEED_MMPS (MISSION_TRACK_SPEED_MMPS * 0.5f)
/* Physical envelope/coverage release is pending D02/D03/D05/D07. The
   synthetic host fixture may explicitly enable the policy to exercise it. */
#ifndef MISSION_BYPASS_RELEASED
#define MISSION_BYPASS_RELEASED 0
#endif
/* Bypass (D02, D12; curved sections out of scope until D12 is answered). */
#define MISSION_OBSTACLE_STOP_MM	150u	/* user-confirmed trigger distance, D16 */
#define MISSION_STATIONARY_MS		150u	/* encoders unchanged this long before a scan */
#define MISSION_BYPASS_OFFSET_MM	250.0f	/* PROVISIONAL (D02): sideways offset to the parallel leg */
#define MISSION_BYPASS_MIN_CLEAR_MM	300u	/* PROVISIONAL (D02): side clearance needed to try */
#define MISSION_BYPASS_TIMEOUT_MS	90000u	/* whole bypass must finish by then */
#define BYPASS_TURN_SPEED_MMPS 7u /* existing bypass pivot speed */
#define BYPASS_LINE_SPEED_MMPS 5u /* D07: scaled final line alignment */

/* DOUT commissioning defaults, NOT calibrated limits (D02/D03/D04/D07).
   Zero measurement fields deliberately inhibit navigation. */
/* Distance-based digital steering (reference-style): a strong fixed correction
   while one sensor is black, bend memory from repeated same-side touches, and a
   gentle drift toward the last side on white/white. Starting values copy the
   reference repo's wheel ratios under this S2 mix; trial values, not calibrated. */
#define LINE_EDGE_Q 115 /* PROVISIONAL D07 trial: one sensor black, ~90% inner-wheel cut */
#define LINE_BEND_EDGE_Q 127 /* PROVISIONAL D07 trial: touch in a bend, inner wheel stopped */
#define LINE_BEND_LEAN_Q 57 /* PROVISIONAL D07 trial: white/white in a bend, ~45% inner cut */
#define LINE_DRIFT_Q 8 /* PROVISIONAL D07 trial: white/white drift toward the last side */
#define LINE_BEND_TOUCH_MM 50u /* PROVISIONAL D07 trial: same-side re-touch or touch this long is a bend */
#define LINE_LOST_MM 200u /* PROVISIONAL D07 trial: white/white this far past the last touch is loss; drift touches within ~50 mm */
#define LINE_BEND_LEAN_MM 100u /* PROVISIONAL D07 trial: bend lean lasts this far past the last touch */
#define LINE_RELEASE_QPS 1800u
_Static_assert(LINE_DRIFT_Q >= 0 && LINE_DRIFT_Q < LINE_BEND_LEAN_Q && LINE_BEND_LEAN_Q < LINE_EDGE_Q &&
    LINE_EDGE_Q <= LINE_BEND_EDGE_Q && LINE_BEND_EDGE_Q <= 127 && LINE_BEND_TOUCH_MM > 0u &&
    LINE_BEND_LEAN_MM >= LINE_BEND_TOUCH_MM && LINE_LOST_MM > LINE_BEND_LEAN_MM, "line steering ordering");
#ifndef NAV_GEOMETRY_RELEASED
#define NAV_GEOMETRY_RELEASED 0
#endif
#ifndef NAV_SENSOR_FORWARD_MM
#define NAV_SENSOR_FORWARD_MM 120 /* User-measured axle centre to IR pair centre after an 18 mm move (INT-117); specs D02 */
#endif
#ifndef NAV_CROSSBAR_MM
#define NAV_CROSSBAR_MM 18 /* User-reported stripe thickness; specs D02 */
#endif
#define NAV_ENTRY_MAX_DEG 5.0f
#define NAV_CONFIRM_MM 9u /* PROVISIONAL D02: half the 18 mm crossing */
#define NAV_ONSET_SKEW_MM 15u /* PROVISIONAL D02 trial: both sensors went black within this = crossing; set from line traces */
#define NAV_REARM_MM 10u
#define NAV_HEADING_REF_MM 100u /* PROVISIONAL D02: oldest straight heading used to cross */
#define NAV_EXIT_MAX_MM 150u
/* Junction pivots end on the line: the leading sensor black then not black,
   looked for only past the sense angle so the branch being left never counts.
   Reference-repo values, PROVISIONAL D02/D07 trial. */
#define NAV_TURN_MAX_DEG 120.0f
#define NAV_TURN_SENSE_DEG 60.0f
#define NAV_UTURN_MAX_DEG 210.0f
#define NAV_UTURN_SENSE_DEG 150.0f
#define NAV_PAUSE_MS 100u
#define NAV_DECEL_MS 500u
#ifndef BARCODE_DIAGNOSTICS
#define BARCODE_DIAGNOSTICS 0
#endif

/* --- S3 barcode ---------------------------------------------------- */
#define BARCODE_GPIO		16u
/* Widths are encoder counts (mean of both wheels). UNMEASURED PLACEHOLDERS
   until #59 measures print scale, polarity and quiet zones (D01, D04). */
#define BARCODE_BLACK_HIGH	0	/* PROVISIONAL (D04): 1 if the module outputs high on black */
#define BARCODE_NARROW_MIN	4u	/* PROVISIONAL (D01) */
#define BARCODE_NARROW_MAX	40u	/* PROVISIONAL (D01) */
#define BARCODE_WIDE_RATIO_MIN_MILLI 2500u	/* PROVISIONAL (D01): observed about 3:1 */
#define BARCODE_WIDE_RATIO_MAX_MILLI 3500u	/* PROVISIONAL (D01) */
#define BARCODE_TOLERANCE_MILLI	300u	/* PROVISIONAL (D01) */
#define BARCODE_QUIET_COUNTS	80u	/* PROVISIONAL (D01): minimum white before a frame */
#define BARCODE_REARM_COUNTS	160u	/* PROVISIONAL (D01): white before the next frame */
#define BARCODE_MAX_ELEMENT_COUNTS 140u	/* PROVISIONAL (D01): longest bar or space */
#define BARCODE_EVENT_CAPACITY	8u
#define BARCODE_EDGE_CAPACITY	64u	/* power of two: indices wrap unsigned */
#define BARCODE_SERVICE_BUDGET	16u

/* --- S4 terrain ---------------------------------------------------- */
#define TERRAIN_CALIBRATION_PERIOD_MS 10u
#define TERRAIN_CALIBRATION_TIMEOUT_MS 5000u /* software budget: 100 samples at
                                              50 Hz plus scheduling margin */
#define TERRAIN_PERIOD_MS	20u
#define GY511_TIMEOUT_US	5000u
#define ACCEL_MG_PER_LSB	0.002f	/* g per digit: 2 mg/digit at +/- 4 g, high resolution (LSM303DLHC) */
#define LPF_ALPHA		0.20f	/* IIR filter weight; PROVISIONAL (D06) */
#define PITCH_CLIMB_THRESH_DEG	3.5f	/* PROVISIONAL (D06) */
#define PITCH_DESC_THRESH_DEG	-3.5f	/* PROVISIONAL (D06) */
#define IMPACT_THRESHOLD_G	2.5f	/* PROVISIONAL (D06); must stay below the +/- 4 g range */
#define HUMP_RESULT_BITS	7u	/* reported height: 7 bits, all-ones is the invalid code */
#define HUMP_MM_PER_COUNT	1u	/* PROVISIONAL (D06): scale of the reported 7-bit result */
#define HUMP_MIN_HEIGHT_MM	5u	/* PROVISIONAL (D06): smaller rises are not humps */
#define HUMP_FLAT_SAMPLES	10u	/* PROVISIONAL (D06): releases at crest before it counts as flat ground */
#define HUMP_CALIB_FACTOR	1.15f	/* PROVISIONAL (D06) */

/* --- S5 obstacle --------------------------------------------------- */
#define SONAR_REPEAT_MS		60u	/* PROVISIONAL (D05): trigger-to-trigger */
#define SONAR_TIMEOUT_US	30000u	/* no echo after this long */
#define SONAR_MIN_PULSE_US	150u	/* shorter than this is not a real echo */
#define SONAR_NO_ECHO_LIMIT	3u	/* consecutive misses that set the fault bit */
#define SONAR_MM_PER_MS_X10	1715u	/* mm per ms of pulse x10: 343 m/s round trip, PROVISIONAL (D05) */
/* Servo and scan. UNMEASURED PLACEHOLDERS until #60 (D05). */
#define SERVO_PIN		12u
#define SERVO_CENTRE_US		1500u	/* PROVISIONAL (D05): faces forward */
#define SERVO_US_PER_DEG	10u	/* PROVISIONAL (D05): endpoints and scale */
#define SERVO_MAX_DEG		20	/* requested +/-20 degrees; calibrate D05 */
#define SERVO_STEP_MS		50u	/* proposed slow slew: 1 degree per step (D05) */
#define SERVO_SETTLE_MS		300u	/* PROVISIONAL (D05) */
#define OBSTACLE_COARSE_STEP_DEG	5	/* settled readings every 5 degrees, -20..20 */
#define OBSTACLE_FINE_STEP_DEG	5	/* refine edges within the servo limits */
#define OBSTACLE_SONAR_FORWARD_MM	100.0f	/* PROPOSAL: front mount relative to body centre, measure D05 */
#define OBSTACLE_SONAR_LEFT_MM	0.0f	/* PROVISIONAL (D05) */
/* Proposed D05 settings for software verification, not calibrated limits.
   Physical release remains inhibited. */
#define OBSTACLE_SCAN_GROUP_SAMPLES 3u
#define OBSTACLE_SCAN_MIN_VALID 2u
#define OBSTACLE_PROFILE_DEPTH_ENTER_MM 30u
#define OBSTACLE_PROFILE_DEPTH_EXIT_MM 60u
#define OBSTACLE_PROFILE_ADJACENT_RAYS 2u
#define OBSTACLE_PROFILE_RANGE_ERROR_MM 10u /* proposed model input, not sensor accuracy */
#define BYPASS_SPEED_MMPS 9u /* D07: scaled to measured counts/mm */
#define BYPASS_CHUNK_MM 50u
#define BYPASS_TRAVEL_MM 4000u
#define BYPASS_SENSE_TIMEOUT_MS 2000u
#define BYPASS_SENSE_ATTEMPTS 2u
#define BYPASS_BODY_WIDTH_MM 180u
#define BYPASS_BODY_LENGTH_MM 200u
#define BYPASS_CLEARANCE_MM 30u
#define BYPASS_MAX_OFFSET_MM 600u
#define BYPASS_FRONT_RESERVE_MM 50u
#define BYPASS_FRONT_MAX_AGE_MS 400u
#define BYPASS_PASS_MAX_MM 1500u
#define BYPASS_BASELINE_SEARCH_MM 250u
#define BYPASS_SIDE_PRESENT_MM 500u
#define BYPASS_EDGE_SAMPLES 2u
#define BYPASS_EDGE_ENTER_MM 150u
#define BYPASS_EDGE_EXIT_MM 100u
#define BYPASS_RETURN_LATERAL_MM 25u
#define BYPASS_RETURN_HEADING_DEG 5u
#define BYPASS_REACQUIRE_MM 100u
#define MISSION_SCAN_RETRIES 1u
#define TERRAIN_QUALITY_MIN_MG 900u
#define TERRAIN_QUALITY_MAX_MG 1100u
#define TERRAIN_QUALITY_SPEED_CHANGE_MMPS 30u
#define TERRAIN_QUALITY_WHEEL_DIFF_MMPS 30u
#define TERRAIN_QUALITY_STEADY_MS 200u
#define OBSTACLE_EXPECTED_MAX_WIDTH_MM 200u /* supplied obstacle width upper bound */
#define OBSTACLE_SCAN_DEADLINE_MS	20000u	/* includes slow slew and per-ray settling */
#define ECHO_PIN		3	/* PROVISIONAL (D05): <= 3.3 V before connecting */
#define TRIGGER_PIN		2

/* --- Shared inputs and locks ---------------------------------------- */
#define BUTTON_GP20		20u
#define BUTTON_GP21		21u
#define BUTTON_DEBOUNCE_MS	30u
#define RD_SNAPSHOT_LOCK_MS	2u	/* bounded wait for a mutex-guarded snapshot copy */

/* --- Relations ----------------------------------------------------- */
_Static_assert(MOTION_MAX_DUTY > 0.0f && MOTION_MAX_DUTY <= 1.0f,
	"motion duty limit must be a fraction");
_Static_assert(MOTION_EXPIRY_MS > 2u * FOLLOW_PERIOD_MS,
	"motion sample age limit must span at least two follow releases");
_Static_assert(IR_MAX_GAP_MS > IR_SAMPLE_MS, "IR gap limit below sample period");
_Static_assert(IR_BLACK_LEVEL <= 1u, "digital black level must be 0 or 1");
_Static_assert((BARCODE_EDGE_CAPACITY & (BARCODE_EDGE_CAPACITY - 1u)) == 0u,
	"barcode edge ring capacity must be a power of two");
_Static_assert(BARCODE_NARROW_MIN <= BARCODE_NARROW_MAX && BARCODE_NARROW_MAX < BARCODE_QUIET_COUNTS &&
	BARCODE_QUIET_COUNTS <= BARCODE_REARM_COUNTS &&
	BARCODE_MAX_ELEMENT_COUNTS > BARCODE_NARROW_MAX * 3u && BARCODE_REARM_COUNTS < ENC_COUNT_MASK / 2u,
	"barcode width limits out of order or beyond the encoder wrap");
_Static_assert(BARCODE_WIDE_RATIO_MIN_MILLI > 1000u && BARCODE_WIDE_RATIO_MIN_MILLI <= BARCODE_WIDE_RATIO_MAX_MILLI &&
	BARCODE_TOLERANCE_MILLI > 0u && BARCODE_TOLERANCE_MILLI < 500u &&
	(unsigned long long)BARCODE_WIDE_RATIO_MIN_MILLI * (1000u - BARCODE_TOLERANCE_MILLI) >
	1000ull * (1000u + BARCODE_TOLERANCE_MILLI),
	"narrow and wide classes overlap at the configured tolerance");
_Static_assert(BARCODE_EVENT_CAPACITY >= 1u, "barcode event queue empty");
_Static_assert(ENC_COUNT_BITS >= 8u && ENC_COUNT_BITS <= 32u, "encoder count width out of range");
_Static_assert((LEFT_ENCODER_SIGN == 1 || LEFT_ENCODER_SIGN == -1) &&
	(RIGHT_ENCODER_SIGN == 1 || RIGHT_ENCODER_SIGN == -1), "encoder sign must be +1 or -1");
_Static_assert(MOTION_SPEED_MAX_MMPS < 511.0f, "commanded speed must fit the 9-bit command field");
_Static_assert(MOTION_KF >= 0.0f && MOTION_KS >= 0.0f && MOTION_KS < MOTION_MAX_DUTY,
	"feedforward terms must be non-negative and below the duty limit");
_Static_assert(MOTION_SPEED_MAX_MMPS > 0.0f && ENC_COUNTS_PER_MM > 0.0f &&
	TRACK_WIDTH_MM > 0.0f, "motion geometry must be positive");
_Static_assert(MOTION_TARGET_MAX_MM * ENC_COUNTS_PER_MM < ENC_COUNT_MASK / 2u,
    "distance targets must fit encoder signed half range");
_Static_assert(MOTION_STALL_MS < MOTION_ACTION_TIMEOUT_MS, "stall limit must precede the action timeout");
_Static_assert(MOTION_FOLLOW_SLEW_MMPS2 > 0u &&
    MOTION_FOLLOW_SLEW_MMPS2 <= 1000u,
    "FOLLOW slew must be positive and fit bounded arithmetic");
_Static_assert(MOTION_SPEED_MAX_MMPS * 64.0f < 32767u,
    "FOLLOW base history must fit 15-bit Q6 storage");
_Static_assert(MOTION_FOLLOW_STEER_QPS > 0u &&
    MOTION_FOLLOW_STEER_QPS <= 127000u &&
    MOTION_FOLLOW_STEER_QPS * MOTION_PERIOD_MS >= 1000u &&
    MISSION_TRACK_SPEED_MMPS > 0.0f &&
    MISSION_TRACK_SPEED_MMPS <= MOTION_SPEED_MAX_MMPS,
    "tracking speed and steering step must remain bounded");
/* User hard guardrail, 2026-11-02: line following, crossings and junction turns
   run at 100 mm/s (half on a hump, per specs); recovery and bypass stay slow. */
_Static_assert(MISSION_TRACK_SPEED_MMPS == 100.0f, "course speed is fixed at 100 mm/s");
_Static_assert(MISSION_TRACK_SPEED_MMPS * 2.0f <= MOTION_SPEED_MAX_MMPS,
	"Mission speeds must stay within the Motion speed limit");
_Static_assert(SONAR_TIMEOUT_US / 1000u < SONAR_REPEAT_MS && SONAR_MIN_PULSE_US < SONAR_TIMEOUT_US &&
	SONAR_NO_ECHO_LIMIT >= 1u, "sonar timing: the timeout must fit inside the repeat interval");
_Static_assert(SERVO_MAX_DEG > 0 && SERVO_MAX_DEG * (int)SERVO_US_PER_DEG < (int)SERVO_CENTRE_US &&
	SERVO_MAX_DEG > OBSTACLE_COARSE_STEP_DEG &&
	OBSTACLE_COARSE_STEP_DEG >= OBSTACLE_FINE_STEP_DEG && OBSTACLE_FINE_STEP_DEG > 0, "servo sweep out of range");
_Static_assert(MOTION_PRI < FOLLOW_PRIORITY,
    "Motion must outrank Mission");
_Static_assert(IO_PRIORITY > 4, "I/O stays below kernel WiFi/USB services");
_Static_assert(SAFETY_STALE_MS >= IR_SAMPLE_MS &&
    FOLLOW_PERIOD_MS <= SAFETY_STALE_MS && MOTION_PERIOD_MS <= SAFETY_STALE_MS,
    "provisional producer limits must cover their release intervals");
_Static_assert(GY511_TIMEOUT_US < IO_PERIOD_MS * 1000u,
    "one I2C operation must not consume a whole I/O release");
_Static_assert(PITCH_DESC_THRESH_DEG < 0.0f && PITCH_CLIMB_THRESH_DEG > 0.0f,
	"pitch thresholds must straddle zero");
_Static_assert(COMMS_TELEMETRY_PERIOD_MS % COMMS_REPORT_PERIOD_MS == 0u && COMMS_EVENT_CAPACITY >= 4u,
	"telemetry period must be a whole number of Reporting releases");
_Static_assert(MISSION_CONFIRM_SAMPLES > 0u && MISSION_CONFIRM_SAMPLES <= 7u,
    "confirmation counters use three bits");
_Static_assert(BYPASS_TRAVEL_MM <= 4095u && BYPASS_SENSE_ATTEMPTS <= 3u &&
    BYPASS_EDGE_SAMPLES > 0u && BYPASS_EDGE_SAMPLES <= 7u,
    "bounded bypass storage must fit its fields");
_Static_assert(RECOVERY_SPEED_MMPS > 0u &&
    RECOVERY_SPEED_MMPS <= MOTION_SPEED_MAX_MMPS &&
    RECOVERY_TARGET_DEG > 0 && RECOVERY_TARGET_DEG <= 175 &&
    RECOVERY_TARGET_TOL_DEG > 0 &&
    RECOVERY_TARGET_TOL_DEG < RECOVERY_TARGET_DEG &&
    RECOVERY_OVERSHOOT_DEG >= RECOVERY_TARGET_TOL_DEG &&
    RECOVERY_TARGET_DEG + RECOVERY_OVERSHOOT_DEG <= 180 &&
    RECOVERY_TRAVEL_MM * ENC_COUNTS_PER_MM < ENC_COUNT_MASK / 2u,
    "recovery pivots must fit Motion targets and encoder half range");
_Static_assert(BYPASS_LINE_SPEED_MMPS > 0u &&
    BYPASS_LINE_SPEED_MMPS * 2u <= MOTION_SPEED_MAX_MMPS,
    "bypass line steering must keep both wheel targets within the speed limit");
_Static_assert(MISSION_SCAN_RETRIES <= 1u, "one bounded stationary rescan");
#endif /* ROBOT_CONFIG_H */
