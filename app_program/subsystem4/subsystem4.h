/*
 * subsystem4.h - Buddy 4 (Terrain) interface definitions.
 */

#ifndef SUBSYSTEM4_H
#define SUBSYSTEM4_H

#include "../app.h"
#include "terrain_algo.h"
#include "terrain_pack.h"
#include "terrain_evidence.h"
#include "../common/robot_config.h"


/* Subsystem lifecycle called by app_main.c */
ER subsystem4_init(void);

/* Terrain word (see terrain_pack.h): hump count and heights in one load. */
uint32_t s4_terrain_word(void);
/* All fields belong to one acquisition/service publication, copied under
   a short cross-core mutex. Repeated reads never change sampled_ms. */

void s4_step(uint32_t now_ms);
bool s4_copy(S4Snapshot *out);
bool s4_startup_complete(void);

#endif /* SUBSYSTEM4_H */