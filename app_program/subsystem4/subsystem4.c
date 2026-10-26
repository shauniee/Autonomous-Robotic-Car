/*
 * subsystem4.c - Buddy 4 (Terrain): lifecycle, terrain word and the 20 ms terrain task.
 */

#include "subsystem4.h"
#include "drv_gy511.h"
#include "../common/robot_data.h"
#include "../common/robot_time.h"
#include "../subsystem1/subsystem1.h"
#include "../subsystem2/drv_encoder.h"

static rd_word_t s_terrain_word;	/* writer: the terrain task; layout in terrain_pack.h */

uint32_t s4_terrain_word(void) { return rd_word_read(&s_terrain_word); }
void s4_publish_word(uint32_t word) { rd_word_publish(&s_terrain_word, word); }

static TerrainContext s_ctx;

static void terrain_task(INT stacd, void *exinf) {
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
    uint8_t prev_hump_count = s_ctx.hump_count;

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

            /* Run-stopping fault: Safety latches the stop. */
            if (s_ctx.state == TERRAIN_STATE_IMPACT) {
                rd_fault_set(RD_OWNER_TERRAIN, RD_FAULT_IMPACT);
            } else {
                rd_fault_clear(RD_OWNER_TERRAIN, RD_FAULT_IMPACT);
            }
            if (s_ctx.hump_count != prev_hump_count) {
                prev_hump_count = s_ctx.hump_count;
                s1_event(MQTT_EVENT_HUMP, s_ctx.last_hump_code, 0u);
            }

            s4_publish_word(terrain_pack(s_ctx.last_hump_code, s_ctx.max_hump_code, s_ctx.hump_count,
                                         (uint32_t)s_ctx.state, s_ctx.height_valid, s_ctx.is_calibrated));
        }

        /* Fixed-rate release every TERRAIN_PERIOD_MS */
        tk_dly_tsk(robot_time_next_delay(&release, TERRAIN_PERIOD_MS, robot_time_ms()));
    }
}

ER subsystem4_init(void) {
    return E_OK;
}

ER subsystem4_start(void) {
    /* Spawn Terrain Task on Core 1 (TP_PRC2) at Priority 16 */
    ID tid = app_start_task((FP)terrain_task, S4_PRI_TERRAIN, TP_PRC2, "terrain_task");
    return (tid < E_OK) ? tid : E_OK;
}
