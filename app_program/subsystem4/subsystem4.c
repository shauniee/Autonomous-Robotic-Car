#include "subsystem4.h"
#include "task_terrain.h"
#include "../subsystem2/subsystem2.h"
#include "../common/robot_time.h"
#include <stdatomic.h>
static ID terrain_mutex;
static TerrainSnapshot latest;
static bool reset_pending, reset_new_run;
static uint32_t reset_epoch;
static bool calibration_pending, calibration_cancel, calibration_active;
static uint32_t calibration_id, calibration_deadline;
ER subsystem4_init(void)
{
    T_CMTX mutex = {0};
    mutex.mtxatr = TA_INHERIT;
    terrain_mutex = tk_cre_mtx(&mutex);
    return terrain_mutex < E_OK ? terrain_mutex : E_OK;
}
ER subsystem4_start(void)
{
    ID tid =
        app_start_task((FP)terrain_task, S4_PRI_TERRAIN, TP_PRC2, "Terrain");
    return tid < E_OK ? tid : E_OK;
}
bool s4_publish_snapshot(const TerrainSnapshot *in)
{
    if (in == NULL || terrain_mutex <= 0 ||
        tk_loc_mtx(terrain_mutex, TMO_POL) < E_OK)
    {
        return false;
    }
    latest = *in;
    if (in->calibration.request_id == calibration_id &&
        in->calibration.status != S4_CAL_RUNNING)
    {
        calibration_active = false;
    }
    return tk_unl_mtx(terrain_mutex) >= E_OK;
}
ER s4_get_terrain(TerrainSnapshot *out)
{
    if (out == NULL || terrain_mutex <= 0)
    {
        return E_PAR;
    }
    ER er = tk_loc_mtx(terrain_mutex, TMO_POL);
    if (er < E_OK)
    {
        return er;
    }
    *out = latest;
    return tk_unl_mtx(terrain_mutex);
}
void s4_reset_request(uint32_t epoch, bool new_run)
{
    UW state;
    DI(state);
    reset_epoch = epoch;
    reset_new_run = new_run;
    reset_pending = true;
    EI(state);
}
bool s4_take_reset(uint32_t *epoch, bool *new_run)
{
    UW state;
    DI(state);
    bool pending = reset_pending;
    *epoch = reset_epoch;
    *new_run = reset_new_run;
    reset_pending = false;
    EI(state);
    return pending;
}
ER s4_calibration_request(uint32_t id, uint32_t deadline_ms)
{
#if TK_SUPPORT_SMP
    if (tk_get_prc() != 2)
    {
        return E_CTX;
    }
#endif
    uint32_t now;
    MotionFeedback motion;
    MotionHealth health;
    if (id == 0u || !robot_now_ms(&now) || robot_due(now, deadline_ms) ||
        deadline_ms - now > 5000u)
    {
        return E_PAR;
    }
    if (s2_feedback_copy(&motion) < E_OK || s2_health(&health) < E_OK ||
        !health.stopped || !health.brake_acknowledged ||
        !motion.encoder.valid || !motion.encoder_quiet ||
        now - motion.sampled_ms > 60u)
    {
        return E_BUSY;
    }
    ER result = tk_loc_mtx(terrain_mutex, TMO_POL);
    if (result < E_OK)
    {
        return result;
    }
    if (calibration_pending || calibration_active)
    {
        result = E_BUSY;
    }
    else
    {
        calibration_id = id;
        calibration_deadline = deadline_ms;
        calibration_cancel = false;
        calibration_active = true;
        calibration_pending = true;
    }
    (void)tk_unl_mtx(terrain_mutex);
    return result;
}
ER s4_calibration_cancel(uint32_t id)
{
#if TK_SUPPORT_SMP
    if (tk_get_prc() != 2)
    {
        return E_CTX;
    }
#endif
    ER result = tk_loc_mtx(terrain_mutex, TMO_POL);
    if (result < E_OK)
    {
        return result;
    }
    if (id == 0u || calibration_id != id)
    {
        result =
            E_PAR; /* A late cancellation cannot replace a newer request. */
    }
    else
    {
        calibration_cancel = true;
        calibration_pending = true;
    }
    (void)tk_unl_mtx(terrain_mutex);
    return result;
}
bool s4_take_calibration(uint32_t *id, uint32_t *deadline, bool *cancel)
{
    if (tk_loc_mtx(terrain_mutex, TMO_POL) < E_OK)
    {
        return false;
    }
    bool pending = calibration_pending;
    *id = calibration_id;
    *deadline = calibration_deadline;
    *cancel = calibration_cancel;
    calibration_pending = false;
    (void)tk_unl_mtx(terrain_mutex);
    return pending;
}
void s4_snapshot_run_reset(TerrainSnapshot *s, uint32_t epoch)
{
    s->height_valid = false;
    s->last_hump_height_m = 0.0f;
    s->max_hump_peak_m = 0.0f;
    run_history_reset(&s->humps, epoch);
    s->run_epoch = epoch;
}
