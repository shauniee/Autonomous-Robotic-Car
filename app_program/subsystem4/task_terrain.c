#include "task_terrain.h"
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
    uint32_t next = 0u, calibration_started = 0u;
    float previous_distance = 0.0f, previous_yaw = 0.0f;
    uint32_t previous_sequence = 0u, previous_time = 0u;
    for (;;)
    {
        uint32_t now;
        if (!robot_now_ms(&now)) { (void)s2_stop(); return; }
        MotionFeedback motion = {0};
        bool motion_ok = s2_feedback_copy(&motion) >= E_OK &&
            motion.valid && now - motion.sampled_ms <= 60u;
        uint32_t epoch;
        bool new_run;
        if (s4_take_reset(&epoch, &new_run))
        {
            float maximum = context.max_hump_peak_m;
            if (context.state == TERRAIN_STATE_IMPACT)
            {
                terrain_algo_init(&context);
                if (!new_run) { context.max_hump_peak_m = maximum; }
                calibration_started = now;
            }
            if (new_run) { terrain_algo_reset_run_peak(&context); }
            snapshot.run_epoch = epoch;
            previous_sequence = 0u;
        }
        Gy511RawAxes raw = {0};
        Gy511Result result = hardware ? drv_gy511_poll_accel(&raw) : GY511_ERROR;
        if (result == GY511_NEW)
        {
            if (!context.is_calibrated)
            {
                if (calibration_started == 0u) { calibration_started = now; }
                if (motion_ok && motion.stationary)
                {
                    (void)terrain_algo_calibrate_sample(&context, &raw);
                }
                else
                {
                    context.calib_count = 0u;
                    context.calib_sum = (Vec3f){0};
                }
            }
            float distance = (motion.left_m + motion.right_m) * 0.5f;
            float travel = previous_sequence != 0u ?
                distance - previous_distance : 0.0f;
            bool travel_ok = motion_ok && previous_sequence != 0u &&
                motion.sequence != previous_sequence &&
                now - previous_time <= 60u &&
                fabsf(motion.left_mps - motion.right_mps) <=
                    robot_config.search_mps;
            terrain_algo_process(&context, &raw, travel, travel_ok, now);
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
            snapshot.turn_rate_deg_s = travel_ok ?
                (motion.yaw_deg - previous_yaw) * 1000.0f /
                    (float)(now - previous_time) : 0.0f;
            snapshot.movement = motion.stationary ? TERRAIN_STATIONARY :
                fabsf(snapshot.turn_rate_deg_s) > 5.0f ? TERRAIN_TURNING :
                context.state == TERRAIN_STATE_CLIMBING ? TERRAIN_CLIMB :
                context.state == TERRAIN_STATE_DESCENDING ? TERRAIN_DESCENT :
                context.state == TERRAIN_STATE_IMPACT ? TERRAIN_IMPACT :
                TERRAIN_ACCELERATING;
            previous_distance = distance;
            previous_yaw = motion.yaw_deg;
            previous_sequence = motion.sequence;
            previous_time = now;
            if (context.state == TERRAIN_STATE_IMPACT) { (void)s2_stop(); }
        }
        else if (result == GY511_ERROR)
        {
            snapshot.terrain_valid = false;
            terrain_algo_invalidate_window(&context);
            snapshot.height_valid = false;
        }
        Gy511Result mag = hardware ? drv_gy511_poll_mag(&raw) : GY511_ERROR;
        if (mag == GY511_NEW)
        {
            snapshot.magnetic_ms = now;
            snapshot.magnetic_valid = true;
            snapshot.magnetic_x_gauss = (float)raw.x / 1100.0f;
            snapshot.magnetic_y_gauss = (float)raw.y / 1100.0f;
            snapshot.magnetic_z_gauss = (float)raw.z / 980.0f;
        }
        else if (mag == GY511_ERROR) { snapshot.magnetic_valid = false; }
        if (now - snapshot.timestamp_ms > 250u) { snapshot.terrain_valid = false; }
        if (now - snapshot.magnetic_ms > 250u) { snapshot.magnetic_valid = false; }
        (void)s4_publish_snapshot(&snapshot);
        if (next == 0u) { next = now; }
        next += 20u;
        if (robot_due(now, next)) { next = now + 20u; }
        if (tk_dly_tsk(next - now) < E_OK) { (void)s2_stop(); return; }
    }
}
