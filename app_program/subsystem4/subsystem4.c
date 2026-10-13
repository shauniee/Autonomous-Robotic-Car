/*
 * subsystem4.c - Lifecycle and shared snapshot management for Buddy 4.
 */

#include "subsystem4.h"
#include "task_terrain.h"

static ID s_terrain_mtx = -1;
static TerrainSnapshot s_latest_snapshot;

ER subsystem4_init(void) {
    /* Create priority-inheritance mutex for thread-safe snapshot reads */
    T_CMTX cmtx = {0};
    cmtx.mtxatr = TA_TFIFO | TA_INHERIT;
    s_terrain_mtx = tk_cre_mtx(&cmtx);
    if (s_terrain_mtx < E_OK) {
        return s_terrain_mtx;
    }
    return E_OK;
}

ER subsystem4_start(void) {
    /* Spawn Terrain Task on Core 1 (TP_PRC2) at Priority 16 */
    ID tid = app_start_task((FP)terrain_task, S4_PRI_TERRAIN, TP_PRC2, "terrain_task");
    return (tid < E_OK) ? tid : E_OK;
}

void s4_publish_snapshot(const TerrainSnapshot *in) {
    if (s_terrain_mtx >= E_OK) {
        tk_loc_mtx(s_terrain_mtx, TMO_FEVR);
        s_latest_snapshot = *in;
        tk_unl_mtx(s_terrain_mtx);
    }
}

ER s4_get_terrain(TerrainSnapshot *out) {
    if (s_terrain_mtx < E_OK) return E_NOEXS;
    ER er = tk_loc_mtx(s_terrain_mtx, 5);
    if (er < E_OK) return er;
    *out = s_latest_snapshot;
    tk_unl_mtx(s_terrain_mtx);
    return E_OK;
}
