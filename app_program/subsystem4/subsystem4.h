/*
 * subsystem4.h - Buddy 4 (Terrain) interface definitions.
 */

#ifndef SUBSYSTEM4_H
#define SUBSYSTEM4_H

#include "../app.h"
#include "terrain_algo.h"
#include "terrain_pack.h"
#include "../common/robot_config.h"


/* Subsystem lifecycle called by app_main.c */
ER subsystem4_init(void);
ER subsystem4_start(void);

/* Terrain word (see terrain_pack.h): hump count and heights in one load. */
uint32_t s4_terrain_word(void);
void s4_publish_word(uint32_t word);

#endif /* SUBSYSTEM4_H */