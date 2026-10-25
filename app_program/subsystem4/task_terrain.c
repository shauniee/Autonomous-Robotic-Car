/*
 * task_terrain.c - 20 ms periodic task executing under micro T-Kernel.
 */

#include "task_terrain.h"
#include "../subsystem2/drv_encoder.h"
#include "../common/robot_data.h"
#include "drv_gy511.h"
#include "terrain_algo.h"
#include "subsystem4.h"
#include "../subsystem2/subsystem2.h"
#include "../common/robot_time.h"
#include "../common/robot_config.h"


static TerrainContext s_ctx;

void terrain_task(INT stacd, void *exinf) {
    (void)stacd; (void)exinf;

    terrain_algo_init(&s_ctx);
    bool hw_ok = drv_gy511_init();
    if (!hw_ok) {
        rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_IMU);
    }

    /* Startup calibration: collect 100 samples while resting (2.0s timeout guard) */
    uint32_t calib_attempts = 0;
    uint32_t release = robot_time_ms();
    uint32_t travel_origin = enc_mean(drv_encoder_count(ENCODER_LEFT), drv_encoder_count(ENCODER_RIGHT));
    while (!s_ctx.is_calibrated && calib_attempts < 200) {
        Gy511RawAxes raw = {0};
        /* Calibrate only while stationary: movement restarts the sampling. */
        uint32_t mean_now = enc_mean(drv_encoder_count(ENCODER_LEFT), drv_encoder_count(ENCODER_RIGHT));
        if (mean_now != travel_origin) {
            travel_origin = mean_now;
            terrain_algo_calibration_restart(&s_ctx);
        } else if (hw_ok && drv_gy511_read_accel(&raw)) {
            terrain_algo_calibrate_sample(&s_ctx, &raw);
        }
        calib_attempts++;
        tk_dly_tsk(robot_time_next_delay(&release, 10u, robot_time_ms()));
    }

    if (!s_ctx.is_calibrated) {
        rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_CALIBRATION_FAILED);
    }
    TerrainState prev_state = s_ctx.state;

    for (;;) {
        Gy511RawAxes raw_accel = {0};
        bool read_ok = hw_ok && drv_gy511_read_accel(&raw_accel);

        /* Sensor failure sets the IMU fault bit; a good read clears it. */
        if (read_ok) {
            rd_fault_clear(RD_OWNER_TERRAIN, RD_FAULT_IMU);
        } else {
            rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_IMU);
        }

        if (read_ok) {
            /* Forward travel since the last release, from this task's own encoder origin. */
            uint32_t mean_now = enc_mean(drv_encoder_count(ENCODER_LEFT), drv_encoder_count(ENCODER_RIGHT));
            int32_t moved = enc_travel_signed(mean_now, travel_origin);
            travel_origin = mean_now;
            terrain_algo_process(&s_ctx, &raw_accel, moved > 0 ? (uint32_t)moved : 0u);

            /* Impact latches IMPACT; request one Safety Stop on the transition only */
            if (s_ctx.state == TERRAIN_STATE_IMPACT && prev_state != TERRAIN_STATE_IMPACT) {
                s2_stop();
            }
            prev_state = s_ctx.state;

            /* Update shared snapshot for Mission and Telemetry */
            TerrainSnapshot snap;
            snap.timestamp_ms = robot_time_ms();
            /* Data is valid only once calibration has produced an offset */
            snap.terrain_valid = s_ctx.is_calibrated;
            snap.accel_x_g = s_ctx.filtered_accel.x;
            snap.accel_y_g = s_ctx.filtered_accel.y;
            snap.accel_z_g = s_ctx.filtered_accel.z;
            snap.pitch_deg = s_ctx.pitch_deg;
            snap.roll_deg = s_ctx.roll_deg;
            snap.state = s_ctx.state;
            snap.height_valid = s_ctx.height_valid;
            snap.last_hump_height_m = (float)(s_ctx.last_hump_code * HUMP_MM_PER_COUNT) / 1000.0f;
            snap.max_hump_peak_m = (float)(s_ctx.max_hump_code * HUMP_MM_PER_COUNT) / 1000.0f;
            snap.hump_count = s_ctx.hump_count;

            s4_publish_snapshot(&snap);
            s4_publish_word(terrain_pack(s_ctx.last_hump_code, s_ctx.max_hump_code, s_ctx.hump_count,
                                         (uint32_t)s_ctx.state, s_ctx.height_valid, s_ctx.is_calibrated));
        } else {
            /* Mark sensor invalid if I2C timed out */
            TerrainSnapshot snap = {0};
            snap.terrain_valid = false;
            s4_publish_snapshot(&snap);
        }

        /* Fixed-rate release every TERRAIN_PERIOD_MS */
        tk_dly_tsk(robot_time_next_delay(&release, TERRAIN_PERIOD_MS, robot_time_ms()));
    }
}
