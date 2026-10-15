#include "task_terrain.h"
#include "terrain_motion.h"
#include "subsystem4.h"
#include "../subsystem2/subsystem2.h"
#include "../common/robot_time.h"
#include <math.h>
void terrain_task(INT stacd, void *exinf)
{
    (void)stacd;
    (void)exinf;
    TerrainContext context;
    terrain_algo_init(&context);
    bool hardware = drv_gy511_init();
    TerrainSnapshot snapshot = {0};
    uint32_t next = 0u;
    S4Calibration calibration = {0};
    MotionFeedback previous_motion = {0};
    bool have_previous = false;
#if APP_TEST
    uint32_t report_due = 0u;
#endif
    for (;;)
    {
        uint32_t now;
        if (!robot_now_ms(&now))
        {
            (void)s2_stop();
            return;
        }
        MotionFeedback motion = {0};
        bool copied_motion = s2_feedback_copy(&motion) >= E_OK;
        MotionHealth health = {0};
        bool copied_health = s2_health(&health) >= E_OK;
        bool quiet = copied_motion && copied_health && health.stopped &&
                     health.brake_acknowledged && motion.encoder.valid &&
                     motion.encoder_quiet && now - motion.sampled_ms <= 60u;
        uint32_t epoch;
        bool new_run;
        if (s4_take_reset(&epoch, &new_run))
        {
            if (context.state == TERRAIN_STATE_IMPACT || !hardware)
            {
                hardware = drv_gy511_init();
                terrain_algo_reinitialize(&context, !new_run);
                calibration = (S4Calibration){0};
            }
            if (new_run)
            {
                terrain_algo_reset_run_peak(&context);
                s4_snapshot_run_reset(&snapshot, epoch);
            }
            snapshot.height_valid = false;
            snapshot.calibrated = context.is_calibrated;
            snapshot.terrain_valid = false;
            snapshot.last_hump_height_m = context.last_hump_height_m;
            snapshot.max_hump_peak_m = context.max_hump_peak_m;
            snapshot.run_epoch = epoch;
            have_previous = false;
        }
        uint32_t calibration_id, deadline;
        bool cancel;
        if (s4_take_calibration(&calibration_id, &deadline, &cancel))
        {
            if (cancel &&
                (int32_t)(calibration_id - calibration.request_id) >= 0)
            {
                if (calibration.request_id != calibration_id ||
                    (calibration.status != S4_CAL_FAILED &&
                     calibration.status != S4_CAL_CANCELLED))
                {
                    calibration.request_id = calibration_id;
                    terrain_calibration_cancel(&calibration, &context);
                }
            }
            else if (!cancel)
            {
                if (!hardware)
                {
                    hardware = drv_gy511_init();
                    terrain_algo_reinitialize(&context, true);
                }
                terrain_calibration_begin(&calibration, &context,
                                          calibration_id, deadline, now);
            }
        }
        Gy511RawAxes raw = {0};
        Gy511Result result =
            hardware ? drv_gy511_poll_accel(&raw) : GY511_ERROR;
        terrain_calibration_step(&calibration, &context, &raw, result, quiet,
                                 now);
        snapshot.calibration = calibration;
        snapshot.calibration_samples = context.calib_count;
        snapshot.calibrated = context.is_calibrated;
        if (!context.is_calibrated)
        {
            snapshot.terrain_valid = false;
            snapshot.height_valid = false;
        }
        if (result == GY511_NEW)
        {
            float distance = (motion.left_m + motion.right_m) * 0.5f;
            float previous_distance =
                (previous_motion.left_m + previous_motion.right_m) * 0.5f;
            float travel = have_previous ? distance - previous_distance : 0.0f;
            float turn, acceleration;
            bool encoder_window =
                terrain_motion_measure(&motion, &previous_motion, have_previous,
                                       now, &turn, &acceleration);
            bool travel_ok =
                encoder_window &&
                fabsf(motion.left_mps - motion.right_mps) <=
                    robot_config.search_mps &&
                fabsf(acceleration) <= robot_config.acceleration_threshold_mps2;
            bool running = copied_health && !health.stopped;
            TerrainState previous_state = context.state;
            terrain_algo_process(&context, &raw, travel, travel_ok && running,
                                 now);
            snapshot.timestamp_ms = now;
            ++snapshot.sequence;
            snapshot.calibrated = context.is_calibrated;
            snapshot.terrain_valid = context.is_calibrated;
            snapshot.accel_x_g = context.filtered_accel.x;
            snapshot.accel_y_g = context.filtered_accel.y;
            snapshot.accel_z_g = context.filtered_accel.z;
            snapshot.pitch_deg = context.pitch_deg;
            snapshot.roll_deg = context.roll_deg;
            snapshot.state = context.state;
            snapshot.height_valid = context.height_valid;
            snapshot.last_hump_height_m = context.last_hump_height_m;
            snapshot.max_hump_peak_m = context.max_hump_peak_m;
            if (previous_state == TERRAIN_STATE_DESCENDING &&
                context.state == TERRAIN_STATE_FLAT && context.height_valid)
            {
                (void)run_history_append(
                    &snapshot.humps,
                    (RunEvent){.sampled_at = now,
                               .a = (int32_t)(motion.x_m * 1000.0f),
                               .b = (int32_t)(motion.y_m * 1000.0f),
                               .value = (int32_t)(context.last_hump_height_m *
                                                  1000.0f)});
            }
            snapshot.motion_valid = encoder_window;
            snapshot.turn_rate_deg_s = turn;
            snapshot.movement =
                terrain_movement_classify(&context, &motion, encoder_window,
                                          turn, acceleration, &robot_config);
            previous_motion = motion;
            have_previous = copied_motion;
            if (context.state == TERRAIN_STATE_IMPACT &&
                previous_state != TERRAIN_STATE_IMPACT)
            {
                (void)s2_stop();
            }
        }
        else if (result == GY511_ERROR)
        {
            hardware = false;
            snapshot.terrain_valid = false;
            terrain_algo_require_calibration(&context);
            snapshot.calibrated = false;
            snapshot.height_valid = false;
        }
#if APP_TEST
        if (robot_due(now, report_due))
        {
            tm_printf((UB *)"[TERRAIN] raw x=%d y=%d z=%d sample=%u "
                            "calibrated=%d height_valid=%d\n",
                      (int)raw.x, (int)raw.y, (int)raw.z, (unsigned)result,
                      context.is_calibrated, context.height_valid);
            report_due = now + 500u;
        }
#endif
        Gy511Result mag = hardware ? drv_gy511_poll_mag(&raw) : GY511_ERROR;
        if (mag == GY511_NEW)
        {
            snapshot.magnetic_ms = now;
            snapshot.magnetic_valid = true;
            snapshot.magnetic_x_gauss = (float)raw.x / 1100.0f;
            snapshot.magnetic_y_gauss = (float)raw.y / 1100.0f;
            snapshot.magnetic_z_gauss = (float)raw.z / 980.0f;
        }
        else if (mag == GY511_ERROR)
        {
            snapshot.magnetic_valid = false;
        }
        if (now - snapshot.timestamp_ms > 250u)
        {
            snapshot.terrain_valid = false;
            if (context.is_calibrated)
            {
                hardware = false;
                terrain_algo_require_calibration(&context);
                snapshot.calibrated = false;
                snapshot.height_valid = false;
            }
        }
        if (!context.is_calibrated && calibration.status == S4_CAL_DONE)
        {
            calibration.status = S4_CAL_FAILED;
            calibration.reason = S4_CAL_SENSOR;
            snapshot.calibration = calibration;
        }
        snapshot.calibrated = context.is_calibrated;
        snapshot.calibration_samples = context.calib_count;
        if (now - snapshot.magnetic_ms > 250u)
        {
            snapshot.magnetic_valid = false;
        }
        (void)s4_publish_snapshot(&snapshot);
        if (next == 0u)
        {
            next = now;
        }
        next += 20u;
        if (!robot_now_ms(&now))
        {
            (void)s2_stop();
            return;
        }
        if (robot_due(now, next))
        {
            next = now + 20u;
        }
        if (tk_dly_tsk(next - now) < E_OK)
        {
            (void)s2_stop();
            return;
        }
    }
}
