#ifndef SUBSYSTEM4_INTERNAL_H
#define SUBSYSTEM4_INTERNAL_H

#include "subsystem4.h"

bool s4_publish_snapshot(const TerrainSnapshot *in);
bool s4_take_reset(uint32_t *epoch, bool *new_run);
bool s4_take_calibration(uint32_t *id, uint32_t *deadline, bool *cancel);
void s4_snapshot_run_reset(TerrainSnapshot *snapshot, uint32_t epoch);

#endif
