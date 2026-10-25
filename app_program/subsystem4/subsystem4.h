/*
 * subsystem4.h - Buddy 4 (Terrain) interface definitions.
 */

#ifndef SUBSYSTEM4_H
#define SUBSYSTEM4_H

#include "../app.h"
#include "terrain_algo.h"
#include "terrain_pack.h"
#include "../common/robot_config.h"


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
    uint8_t      hump_count;
} TerrainSnapshot;

/* Subsystem lifecycle called by app_main.c */
ER subsystem4_init(void);
ER subsystem4_start(void);

/* Public thread-safe data access */
ER s4_get_terrain(TerrainSnapshot *out);
/* Terrain word (see terrain_pack.h): hump count and heights in one load. */
uint32_t s4_terrain_word(void);
void s4_publish_word(uint32_t word);
void s4_publish_snapshot(const TerrainSnapshot *in);

#endif /* SUBSYSTEM4_H */