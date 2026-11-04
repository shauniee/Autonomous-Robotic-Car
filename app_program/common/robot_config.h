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

/* --- comms --------------------------------------------- */
/* Diagnostic trace frames 9/10/11 are not required for ordinary telemetry.
 * TRACE=0 removes their high-rate production and draining for A/B line
 * tracking trials; TRACE=1 preserves the existing diagnostic behavior.
 * Both profiles retain telemetry, events, mapping ACKs and safety.
 */
#ifndef ROBOT_TRACE_ENABLED
#define ROBOT_TRACE_ENABLED 1
#endif
_Static_assert(ROBOT_TRACE_ENABLED == 0 || ROBOT_TRACE_ENABLED == 1,
    "ROBOT_TRACE_ENABLED must be boolean");
#define COMMS_TELEMETRY_PERIOD_MS	500u
#define COMMS_REPORT_PERIOD_MS	50u	/* Reporting release: requests and events are served at this rate */
#define COMMS_EVENT_CAPACITY	16u	/* event-log ring; oldest overwritten, drops counted */
#define COMMS_REQUESTS_PER_RELEASE 2u /* bounded by incoming bridge depth */
#define COMMS_EVENTS_PER_RELEASE	2u

/* --- motion ------------------------------------------------------ */
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
/* INT-138: a lone invalid edge costs about 0.2 mm, so it must not stop a run.
   Each one scores a point and each Motion release removes one; reaching this
   score means sustained noise. PROVISIONAL D03. */
#define ENC_FAULT_INVALID_SCORE	8u
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
/* PROVISIONAL (D07): a pivot brakes at the edge of this band, so it is always
   short by about this much (2 degrees measured 88 of 90) */
#define MOTION_ANGLE_TOL_DEG 1.0f
#define MOTION_APPROACH_GAIN	4.0f	/* 1/s: speed = gain * remaining distance */
#define MOTION_STATIONARY_CPS	5.0f	/* PROVISIONAL (D08): below this a wheel is stationary */
#define MOTION_STALL_MS		400u	/* PROVISIONAL (D07) */
#define MOTION_STALL_MIN_TARGET_CPS (10.0f * ENC_COUNTS_PER_MM) /* PROVISIONAL (D07): the end of an approach asks for too little to turn a wheel; INT-156 sat at 3 mm/s for 1060 ms */
#define MOTION_ACTION_TIMEOUT_MS 12000u /* D07 trial: corrected turn distances */
#define MOTION_PERIOD_MS	20u

/* Optional reset watchdog (D15). Off until electrically validated on the
 * mounted robot. Enable explicitly in a controlled bench build, then test
 * full reset, motor pin states and startup inhibition before deployment.
 * RP2040-E1: hardware load needs two watchdog counts per microsecond. */
#ifndef ROBOT_WATCHDOG_ENABLED
#define ROBOT_WATCHDOG_ENABLED 0
#endif
#define ROBOT_WATCHDOG_TIMEOUT_MS 2000u
#define ROBOT_WATCHDOG_PROGRESS_MAX_AGE_MS 250u
_Static_assert(ROBOT_WATCHDOG_ENABLED == 0 || ROBOT_WATCHDOG_ENABLED == 1,
    "watchdog enable is boolean");
_Static_assert(ROBOT_WATCHDOG_TIMEOUT_MS > ROBOT_WATCHDOG_PROGRESS_MAX_AGE_MS &&
    ROBOT_WATCHDOG_TIMEOUT_MS <= 8388u,
    "RP2040 watchdog timeout must fit the 24-bit load");


/* --- line ------------------------------------------------------- */
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
#define MOTION_STALL_GUARD_ENABLED 0 /* the full-course build sets 1 */
#endif
#define RECOVERY_MS 30000u
#define RECOVERY_TRAVEL_MM 1200u /* PROPOSAL D07: wheel excursion bound */
#define RECOVERY_SPEED_MMPS 36u /* D07: preserves pre-calibration count speed */
#define RECOVERY_TARGET_DEG 70 /* user: sweep each side of the stopped pose */
#define RECOVERY_TARGET_TOL_DEG 5
#define RECOVERY_OVERSHOOT_DEG 20 /* PROPOSAL D07/D08: coast before reversal */
#define MISSION_CONFIRM_SAMPLES 3u
#define MOTION_FOLLOW_SLEW_MMPS2 250u /* PROVISIONAL D07: common FOLLOW base ramp */
#define MOTION_FOLLOW_STEER_QPS 900u /* PROVISIONAL D07: applied steering slew */
#define MISSION_TRACK_SPEED_MMPS 100.0f /* user hard guardrail: fixed course speed, not overridable */
/* White reported as black with LOW polarity; use HIGH for black. */
#define IR_BLACK_LEVEL 1u

/* --- Mission. UNMEASURED PLACEHOLDERS until #59/#61 (D01, D02, D07) ----- */
/* Physical envelope/coverage release is pending D02/D03/D05/D07. The
   synthetic host fixture may explicitly enable the policy to exercise it. */
#ifndef MISSION_BYPASS_RELEASED
#define MISSION_BYPASS_RELEASED 0
#endif
/* Bypass (D02, D12; curved sections out of scope until D12 is answered). */
#define MISSION_OBSTACLE_STOP_MM	150u	/* user-confirmed trigger distance, D16 */
#define MISSION_STATIONARY_MS		150u	/* encoders unchanged this long before a scan */
/* User-chosen 2026-11-03: 160 mm measured body + 30 mm each side; unaccepted
   on the car */
#define MISSION_BYPASS_MIN_CLEAR_MM 220u
#define MISSION_BYPASS_TIMEOUT_MS	90000u	/* whole bypass must finish by then */
#define BYPASS_TURN_SPEED_MMPS 36u /* PROVISIONAL D07: recovery pivot speed; 7 mm/s overran the action timeout */

/* DOUT commissioning defaults, NOT calibrated limits (D02/D03/D04/D07).
   Zero measurement fields deliberately inhibit navigation. */
/* Distance-based digital steering (reference-style): a strong fixed correction
   while one sensor is black, bend memory from repeated same-side touches, and a
   gentle drift toward the last side on white/white. Trial values, not calibrated. */
#define LINE_EDGE_Q 60 /* PROVISIONAL D07 trial INT-132: one sensor black; 115 overshot at 100 mm/s */
#define LINE_BEND_EDGE_Q 90 /* PROVISIONAL D07 trial INT-132: touch in a bend */
#define LINE_BEND_LEAN_Q 30 /* PROVISIONAL D07 trial INT-132: white/white in a bend */
#define LINE_DRIFT_Q 8 /* PROVISIONAL D07 trial: white/white drift toward the last side */
#define LINE_BEND_TOUCH_MM 50u /* PROVISIONAL D07 trial: same-side re-touch or touch this long is a bend */
#define LINE_LOST_MM 350u /* PROVISIONAL D07 trial: white/white this far past the last touch is loss; INT-135: the drift arc can stay between the sensors for about 270 mm */
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
#define NAV_CONFIRM_MM 6u /* PROVISIONAL D02 trial INT-189: both black this far is a crossing; shorter reads were not, and real ones can read under 9 mm */
#define NAV_ONSET_SKEW_MM 15u /* PROVISIONAL D02 trial: both sensors went black within this = crossing; set from line traces */
#define NAV_REARM_MM 10u
#define NAV_STRAIGHT_MM 36u /* PROVISIONAL D02 trial INT-189: straight crossing advance from commit, twice the crossbar */
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

/* --- barcode ---------------------------------------------------- */
#define BARCODE_GPIO		16u
/* Widths are encoder counts of sensor travel. UNMEASURED PLACEHOLDERS until
   #59 measures print scale and polarity (D01, D04). */
#define BARCODE_BLACK_HIGH	1	/* PROVISIONAL (D04): as the line modules (IR_BLACK_LEVEL); INT-137 */
#define BARCODE_SENSOR_LEFT_MM	55u	/* User-reported position left of the axle centre; specs D01 */
#define BARCODE_MIN_ELEMENT_COUNTS 16u	/* PROVISIONAL (D01): about 1.4 mm; narrow bars read as low as 23 (INT-152), shorter runs are chatter */
#define BARCODE_MAX_ELEMENT_COUNTS 400u	/* PROVISIONAL (D01): longer runs are open floor; a 2x print's wide bar is about 210 */
#define BARCODE_EVENT_CAPACITY	8u
#define BARCODE_EDGE_CAPACITY	64u	/* power of two: indices wrap unsigned */
#define BARCODE_SERVICE_BUDGET	16u

/* --- terrain ---------------------------------------------------- */
#ifndef TERRAIN_DIAGNOSTICS
#define TERRAIN_DIAGNOSTICS 0
#endif
#define TERRAIN_CALIBRATION_PERIOD_MS 10u
#define TERRAIN_CALIBRATION_TIMEOUT_MS 5000u /* software budget: 100 samples at
                                              50 Hz plus scheduling margin */
#define TERRAIN_PERIOD_MS	20u
#define GY511_TIMEOUT_US	5000u
#define ACCEL_MG_PER_LSB	0.002f	/* g per digit: 2 mg/digit at +/- 4 g, high resolution (LSM303DLHC) */
#define LPF_ALPHA		0.20f	/* IIR filter weight; PROVISIONAL (D06) */
#define HUMP_ROLL_ENTER_DEG	3.5f	/* PROVISIONAL (D06): from the roll average while stopped */
#define HUMP_ROLL_EXIT_DEG	2.0f	/* PROVISIONAL (D06) */
#define IMPACT_THRESHOLD_G	2.5f	/* PROVISIONAL (D06); must stay below the +/- 4 g range */
#define HUMP_RESULT_BITS	7u	/* reported height: 7 bits, all-ones is the invalid code */
#define HUMP_MM_PER_COUNT	1u	/* PROVISIONAL (D06): scale of the reported 7-bit result */
#define HUMP_MIN_HEIGHT_MM	5u	/* PROVISIONAL (D06): smaller rises are not humps */
#define HUMP_MAX_HEIGHT_MM	40u	/* PROVISIONAL (D06): course limit 30 mm plus margin; above it the car was lifted */
#define HUMP_MIN_TRAVEL_MM	50u	/* PROVISIONAL (D06): travel over the exit level; INT-157 humps 76 and 156, curves 26 or less */
#define HUMP_MAX_TRAVEL_MM	400u	/* PROVISIONAL (D06): tilted further than this is new ground */
#define HUMP_FLAT_SAMPLES	10u	/* PROVISIONAL (D06): releases below the exit level that end an encounter */
#define HUMP_CALIB_FACTOR	0.8f	/* PROVISIONAL (D06): one 10 mm hump read about 12.5 uncorrected, INT-157 and INT-158 */

/* --- obstacle --------------------------------------------------- */
#define SONAR_REPEAT_MS		60u	/* PROVISIONAL (D05): trigger-to-trigger */
#define SONAR_TIMEOUT_US	30000u	/* no echo after this long */
#define SONAR_MIN_PULSE_US	150u	/* shorter than this is not a real echo */
#define SONAR_NO_ECHO_LIMIT	3u	/* consecutive misses that set the fault bit */
#define SONAR_MM_PER_MS_X10	1715u	/* mm per ms of pulse x10: 343 m/s round trip, PROVISIONAL (D05) */
/* Servo and scan. UNMEASURED PLACEHOLDERS until #60 (D05). */
#define SERVO_PIN		12u
#define SERVO_CENTRE_US		1500u	/* PROVISIONAL (D05): faces forward */
#define SERVO_US_PER_DEG	10u	/* PROVISIONAL (D05): endpoints and scale */
/* user: the servo can do +/-60 degrees (2026-11-03); calibrate D05 */
#define SERVO_MAX_DEG 60
/* User-chosen side look while passing (2026-11-03), beyond the scan sweep;
   calibrate D05 */
#define SERVO_LOOK_DEG 70
#define SERVO_STEP_MS		10u	/* PROVISIONAL (D05): 1 degree per step, a bypass look must fit BYPASS_SENSE_TIMEOUT_MS */
#define SERVO_SETTLE_MS		100u	/* PROVISIONAL (D05) */
/* settled readings every 10 degrees, -60..60 */
#define OBSTACLE_COARSE_STEP_DEG 10
#define OBSTACLE_FINE_STEP_DEG	5	/* refine edges within the servo limits */
/* User-measured ahead of the axle (2026-11-03); D05 */
#define OBSTACLE_SONAR_FORWARD_MM 105.0f
#define OBSTACLE_SONAR_LEFT_MM	0.0f	/* PROVISIONAL (D05) */
/* Proposed D05 settings for software verification, not calibrated limits.
   Physical release remains inhibited. */
#define OBSTACLE_SCAN_GROUP_SAMPLES 3u
#define OBSTACLE_SCAN_MIN_VALID 2u
#define OBSTACLE_PROFILE_DEPTH_ENTER_MM 30u
#define OBSTACLE_PROFILE_DEPTH_EXIT_MM 40u
#define OBSTACLE_PROFILE_ADJACENT_RAYS 2u
#define OBSTACLE_PROFILE_RANGE_ERROR_MM 10u /* proposed model input, not sensor accuracy */
#define BYPASS_SPEED_MMPS 50u /* PROVISIONAL D07: 9 mm/s overran MISSION_BYPASS_TIMEOUT_MS */
#define BYPASS_TRAVEL_MM 4000u
/* PROVISIONAL D05: slewing 70 degrees is estimated at 1.75 s from scan
   timing */
#define BYPASS_SENSE_TIMEOUT_MS 4000u
/* Outline from the axle centre, user-measured 2026-11-03, assuming the sonar
   is on the centreline. The barcode sensor arm is the front-left corner.
   D03. */
#define BYPASS_BODY_NOSE_MM 155u /* forward, front face */
#define BYPASS_BODY_ARM_FORWARD_MM 183u /* forward, barcode arm corner */
#define BYPASS_BODY_NOSE_RIGHT_MM 39u /* right, front-right corner */
#define BYPASS_BODY_LEFT_MM 85u /* left, barcode arm */
#define BYPASS_BODY_RIGHT_MM 75u
#define BYPASS_BODY_REAR_MM 95u
#define BYPASS_CLEARANCE_MM 30u
/* PROVISIONAL D07: beyond a pivot's reach, for range jumps of about 10 mm
   (S5-U11) */
#define BYPASS_BACKUP_MARGIN_MM 20u
/* PROVISIONAL D07: reversing is blind; total for one obstacle, covers the stop
   distance down to contact */
#define BYPASS_BACKUP_MAX_MM 150u
/* PROVISIONAL D07: reverse this far and rescan when a scan shows no room to
   pass */
#define BYPASS_NO_ROOM_STEP_MM 50u
#define BYPASS_MAX_OFFSET_MM 550u
/* one drive, within MOTION_ACTION_TIMEOUT_MS at BYPASS_SPEED_MMPS */
#define BYPASS_LEG_MAX_MM 550u
/* User-chosen: car side to the measured box edge while passing */
#define BYPASS_SAFE_GAP_MM 10u
/* PROVISIONAL D05: travel with the side ray off the box before it counts as
   clear */
#define BYPASS_EDGE_MM 30u
#define BYPASS_FRONT_MAX_AGE_MS 400u
#define BYPASS_EDGE_SAMPLES 2u
/* side readings in a row inside the car before stopping; one bad ping must
   not */
#define BYPASS_CLOSE_SAMPLES 3u
#define BYPASS_RETURN_HEADING_DEG 5u
#define MISSION_SCAN_RETRIES 1u
/* forward readings in a row, after a sweep, that mean the obstacle has gone */
#define MISSION_SCAN_CLEAR_SAMPLES 3u
/* beyond the stop distance plus any reverse, so a range jump is not a
   removal */
#define MISSION_SCAN_CLEAR_MARGIN_MM 30u
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
_Static_assert(BARCODE_MIN_ELEMENT_COUNTS > 0u && BARCODE_MIN_ELEMENT_COUNTS < BARCODE_MAX_ELEMENT_COUNTS &&
	BARCODE_MAX_ELEMENT_COUNTS < ENC_COUNT_MASK / 2u && BARCODE_SENSOR_LEFT_MM * 2u < (unsigned)TRACK_WIDTH_MM,
	"barcode width limits and sensor offset");
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
   run at 100 mm/s, humps included; recovery and bypass stay slow. */
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
_Static_assert(HUMP_ROLL_ENTER_DEG > HUMP_ROLL_EXIT_DEG && HUMP_ROLL_EXIT_DEG > 0.0f,
	"hump roll exit must sit below entry");
_Static_assert(COMMS_TELEMETRY_PERIOD_MS % COMMS_REPORT_PERIOD_MS == 0u && COMMS_EVENT_CAPACITY >= 4u,
	"telemetry period must be a whole number of Reporting releases");
_Static_assert(MISSION_CONFIRM_SAMPLES > 0u && MISSION_CONFIRM_SAMPLES <= 7u,
    "confirmation counters use three bits");
_Static_assert(BYPASS_BACKUP_MAX_MM <= 255u,
    "Mission stores the reverse in 8 bits");
_Static_assert(BYPASS_TRAVEL_MM <= 4095u && BYPASS_EDGE_SAMPLES > 0u &&
    BYPASS_EDGE_SAMPLES <= 7u,
    "bounded bypass storage must fit its fields");
_Static_assert(BYPASS_LEG_MAX_MM * 1000u / BYPASS_SPEED_MMPS <
    MOTION_ACTION_TIMEOUT_MS && BYPASS_MAX_OFFSET_MM <= BYPASS_LEG_MAX_MM,
    "a bypass leg must finish inside one Motion action");
_Static_assert(SERVO_LOOK_DEG >= SERVO_MAX_DEG &&
    SERVO_LOOK_DEG * (int)SERVO_US_PER_DEG < (int)SERVO_CENTRE_US,
    "side look out of servo range");
_Static_assert(RECOVERY_SPEED_MMPS > 0u &&
    RECOVERY_SPEED_MMPS <= MOTION_SPEED_MAX_MMPS &&
    RECOVERY_TARGET_DEG > 0 && RECOVERY_TARGET_DEG <= 175 &&
    RECOVERY_TARGET_TOL_DEG > 0 &&
    RECOVERY_TARGET_TOL_DEG < RECOVERY_TARGET_DEG &&
    RECOVERY_OVERSHOOT_DEG >= RECOVERY_TARGET_TOL_DEG &&
    RECOVERY_TARGET_DEG + RECOVERY_OVERSHOOT_DEG <= 180 &&
    RECOVERY_TRAVEL_MM * ENC_COUNTS_PER_MM < ENC_COUNT_MASK / 2u,
    "recovery pivots must fit Motion targets and encoder half range");
_Static_assert(MISSION_SCAN_RETRIES <= 1u, "one bounded stationary rescan");
#endif /* ROBOT_CONFIG_H */
