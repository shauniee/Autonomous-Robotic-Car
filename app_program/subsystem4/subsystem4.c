#include "subsystem4.h"
#include "task_terrain.h"
#include <stdatomic.h>
static ID terrain_mutex;
static TerrainSnapshot latest;
static atomic_uint reset_request;
ER subsystem4_init(void)
{
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    terrain_mutex = tk_cre_mtx(&mutex);
    return terrain_mutex < E_OK ? terrain_mutex : E_OK;
}
ER subsystem4_start(void)
{
    ID tid = app_start_task((FP)terrain_task, S4_PRI_TERRAIN,
        TP_PRC2, "Terrain");
    return tid < E_OK ? tid : E_OK;
}
bool s4_publish_snapshot(const TerrainSnapshot *in)
{
    if (in == NULL || terrain_mutex <= 0 ||
        tk_loc_mtx(terrain_mutex, TMO_POL) < E_OK) { return false; }
    latest = *in;
    return tk_unl_mtx(terrain_mutex) >= E_OK;
}
ER s4_get_terrain(TerrainSnapshot *out)
{
    if (out == NULL || terrain_mutex <= 0) { return E_PAR; }
    ER er = tk_loc_mtx(terrain_mutex, TMO_POL);
    if (er < E_OK) { return er; }
    *out = latest;
    return tk_unl_mtx(terrain_mutex);
}
void s4_reset_request(uint32_t epoch, bool new_run)
{
    atomic_store_explicit(&reset_request, (epoch << 2u) | 1u |
        (new_run ? 2u : 0u), memory_order_release);
}
bool s4_take_reset(uint32_t *epoch, bool *new_run)
{
    uint32_t request = atomic_exchange_explicit(&reset_request, 0u,
        memory_order_acq_rel);
    if (request == 0u) { return false; }
    *epoch = request >> 2u;
    *new_run = (request & 2u) != 0u;
    return true;
}
