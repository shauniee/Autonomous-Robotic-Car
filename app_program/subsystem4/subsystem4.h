#ifndef SUBSYSTEM4_H
#define SUBSYSTEM4_H
#include "../app.h"
#include "terrain_algo.h"
#include "terrain_calibration.h"
#include "../common/run_history.h"
#define S4_PRI_TERRAIN 16
typedef struct
{
    uint32_t timestamp_ms, sequence, magnetic_ms, run_epoch;
    bool terrain_valid, calibrated, magnetic_valid, height_valid, motion_valid;
    float accel_x_g, accel_y_g, accel_z_g, pitch_deg, roll_deg;
    float magnetic_x_gauss, magnetic_y_gauss, magnetic_z_gauss;
    float turn_rate_deg_s, last_hump_height_m, max_hump_peak_m;
    TerrainState state;
    TerrainMovement movement;
    S4Calibration calibration;
    uint16_t calibration_samples;
    RunHistory humps;
} TerrainSnapshot;
ER subsystem4_init(void);
ER subsystem4_start(void);
ER s4_get_terrain(TerrainSnapshot *out);
/* Core-1 request; processing owns all estimator state. */
void s4_reset_request(uint32_t run_epoch, bool new_run);
/* Core-1 nonblocking handoff. Only explicit black capture requests calibrate.
 */
ER s4_calibration_request(uint32_t id, uint32_t deadline_ms);
ER s4_calibration_cancel(uint32_t id);
#endif
