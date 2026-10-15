#include "subsystem1.h"
#include "../subsystem3/mission.h"
#include "../subsystem3/subsystem3.h"
#include "../subsystem2/subsystem2.h"
#include "../subsystem4/subsystem4.h"
#include "../subsystem5/subsystem5.h"
#include "../common/robot_time.h"
#include <math.h>
#include <string.h>
static int32_t milli(float value)
{
    if (!isfinite(value) || fabsf(value) >= 2147483.0f)
    {
        return 0;
    }
    return (int32_t)(value * 1000.0f);
}
static void label(char *to, unsigned capacity, const char *from)
{
    unsigned count = (unsigned)strlen(from);
    if (count >= capacity)
    {
        count = capacity - 1u;
    }
    memcpy(to, from, count);
    to[count] = '\0';
}
void s1_reporting_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    static S1Telemetry previous;
    for (;;)
    {
        uint32_t now;
        if (!robot_now_ms(&now))
        {
            return;
        }
        S1Telemetry s = {.observed_ms = now, .test_inhibited = APP_TEST != 0};
        S3IrSnapshot ir = {0};
        MissionSnapshot mission = {0};
        MotionFeedback motion = {0};
        TerrainSnapshot terrain = {0};
        ObstacleSnapshot obstacle = {0};
        barcode_status_t barcode = {0};
        if (s3_ir_copy(&ir))
        {
            s.sampled_ms = ir.sampled_ms;
            s.have_sample = ir.have_sample;
            s.acquired = ir.acquisition_valid;
            s.reading_valid = ir.normalized_valid;
            s.raw_left = ir.raw.left;
            s.raw_right = ir.raw.right;
            s.black_set = ir.calibration.black_set;
            s.white_set = ir.calibration.white_set;
            s.black_left = ir.calibration.black.left;
            s.black_right = ir.calibration.black.right;
            s.white_left = ir.calibration.white.left;
            s.white_right = ir.calibration.white.right;
            s.left_fraction =
                (int16_t)milli((ir.normalized_left + 1.0f) * 0.5f);
            s.right_fraction =
                (int16_t)milli((ir.normalized_right + 1.0f) * 0.5f);
            s.error = (int16_t)milli(ir.error);
            label(s.line, sizeof(s.line), ir_tracking_line_name(ir.line));
            label(s.capture, sizeof(s.capture),
                  ir_tracking_capture_name(ir.capture_result));
        }
        if (mission_copy(&mission))
        {
            label(s.state, sizeof(s.state), mission_state_name(mission.state));
            label(s.reason, sizeof(s.reason),
                  mission_reason_name(mission.reason));
            s.run_epoch = mission.run_epoch;
            run_history_reset(&s.humps, s.run_epoch);
            run_history_reset(&s.barcodes, s.run_epoch);
            if (previous.run_epoch == s.run_epoch)
            {
                s.humps = previous.humps;
                s.barcodes = previous.barcodes;
                s.terrain_run_epoch = previous.terrain_run_epoch;
                s.max_height_mm = previous.max_height_mm;
            }
            s.action_id = mission.action_id;
            s.completed_actions = mission.completed_actions;
            s.configured = mission.configured;
            s.motion_enabled = mission.moving;
            s.safety_latched = mission.safety_latched;
            s.mapping_request_id = mission.mapping_request_id;
            s.mapping_result = mission.mapping_result;
        }
        else
        {
            /* Missing Mission identity cannot relabel another run's metrics. */
            if (tk_dly_tsk(100u) < E_OK)
            {
                return;
            }
            continue;
        }
        if (s2_feedback_copy(&motion) >= E_OK)
        {
            s.motion_ms = motion.sampled_ms;
            s.motion_sequence = motion.sequence;
            s.motion_valid = motion.valid;
            s.encoder_valid = motion.encoder.valid;
            s.stationary = motion.stationary;
            s.left_count = motion.encoder.left_count;
            s.right_count = motion.encoder.right_count;
            s.left_mm = milli(motion.left_m);
            s.right_mm = milli(motion.right_m);
            s.left_mmps = milli(motion.left_mps);
            s.right_mmps = milli(motion.right_mps);
            s.x_mm = milli(motion.x_m);
            s.y_mm = milli(motion.y_m);
            s.yaw_mdeg = milli(motion.yaw_deg);
            s.output_left = (int16_t)milli(motion.left_duty);
            s.output_right = (int16_t)milli(motion.right_duty);
            s.motion_status = motion.status;
            s.motion_reason = motion.reason;
        }
        if (s4_get_terrain(&terrain) >= E_OK)
        {
            if (terrain.run_epoch == s.run_epoch)
            {
                s.humps = terrain.humps;
            }
            s.calibration_status = terrain.calibration.status;
            s.calibration_samples = terrain.calibration_samples;
            s.calibration_reason = terrain.calibration.reason;
            s.terrain_run_epoch = terrain.run_epoch;
            s.terrain_ms = terrain.timestamp_ms;
            s.terrain_sequence = terrain.sequence;
            s.magnetic_ms = terrain.magnetic_ms;
            s.terrain_valid = terrain.terrain_valid;
            s.terrain_motion_valid = terrain.motion_valid;
            s.terrain_calibrated = terrain.calibrated;
            s.height_valid = terrain.height_valid;
            s.magnetic_valid = terrain.magnetic_valid;
            s.accel_mg[0] = milli(terrain.accel_x_g);
            s.accel_mg[1] = milli(terrain.accel_y_g);
            s.accel_mg[2] = milli(terrain.accel_z_g);
            s.magnetic_milli_gauss[0] = milli(terrain.magnetic_x_gauss);
            s.magnetic_milli_gauss[1] = milli(terrain.magnetic_y_gauss);
            s.magnetic_milli_gauss[2] = milli(terrain.magnetic_z_gauss);
            s.pitch_mdeg = milli(terrain.pitch_deg);
            s.roll_mdeg = milli(terrain.roll_deg);
            s.turn_mdeg_s = milli(terrain.turn_rate_deg_s);
            s.height_mm = milli(terrain.last_hump_height_m);
            s.max_height_mm = milli(terrain.max_hump_peak_m);
            s.terrain_state = terrain.state;
            s.movement = terrain.movement;
        }
        if (s5_obstacle_copy(&obstacle) >= E_OK)
        {
            s.front_ms = obstacle.front_ms;
            s.profile_ms = obstacle.profile_ms;
            s.scan_id = obstacle.scan_id;
            s.scan_status = obstacle.scan_status;
            s.front_valid = obstacle.front_valid;
            s.profile_valid = obstacle.profile_valid;
            s.front_mm = milli(obstacle.front_m);
            s.closest_mm = milli(obstacle.closest_m);
            s.width_mm = milli(obstacle.width_m);
            s.left_gap_mm = milli(obstacle.left_clearance_m);
            s.right_gap_mm = milli(obstacle.right_clearance_m);
        }
        if (s3_barcode_copy(&barcode) == BARCODE_API_OK)
        {
            if (barcode.encounters.run_epoch == s.run_epoch)
            {
                s.barcodes = barcode.encounters;
                if (s.barcodes.length > 0u)
                {
                    const RunEvent *last =
                        &s.barcodes.events[s.barcodes.length - 1u];
                    s.symbol[0] = (char)last->value;
                    s.navigation = last->detail >> 1u;
                    s.orientation = last->detail & 1u;
                }
            }
            s.mapping_revision = barcode.mapping_revision;
            s.scanner_epoch = barcode.scanner_epoch;
            s.barcode_faults = barcode.fault_bits;
        }
        if (s.barcodes.length > 0u && s.barcodes.length <= RUN_HISTORY_CAPACITY)
        {
            const RunEvent *last = &s.barcodes.events[s.barcodes.length - 1u];
            s.symbol[0] = (char)last->value;
            s.navigation = last->detail >> 1u;
            s.orientation = last->detail & 1u;
        }
        previous = s;
        (void)s1_telemetry_update(&s);
        if (tk_dly_tsk(100u) < E_OK)
        {
            return;
        }
    }
}
