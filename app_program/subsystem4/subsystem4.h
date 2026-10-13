/*
 * subsystem4.h - Buddy 4 (Terrain) interface definitions.
 */

#ifndef SUBSYSTEM4_H
#define SUBSYSTEM4_H

#include "../app.h"
#include "terrain_algo.h"

#define S4_PRI_TERRAIN  16      /* Priority 16, Core 1 (TEAM.md section 4) */

typedef struct {
    uint32_t     timestamp_ms;
    bool         terrain_valid;
    float        accel_x_g;          /* Calibrated lateral acceleration (g) */
    float        accel_y_g;          /* Calibrated forward acceleration (g) */
    float        accel_z_g;          /* Calibrated vertical acceleration (g) */
    float        pitch_deg;
    float        roll_deg;
    TerrainState state;
    bool         height_valid;
    float        last_hump_height_m;
    float        max_hump_peak_m;
} TerrainSnapshot;

/* Subsystem lifecycle called by app_main.c */
ER subsystem4_init(void);
ER subsystem4_start(void);

/* Public thread-safe data access */
ER s4_get_terrain(TerrainSnapshot *out);
void s4_publish_snapshot(const TerrainSnapshot *in);

#endif /* SUBSYSTEM4_H */