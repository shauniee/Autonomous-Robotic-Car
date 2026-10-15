#include "terrain_calibration.h"
#include "../common/robot_time.h"
static void fail(S4Calibration *c, TerrainContext *context,
                 S4CalibrationReason reason)
{
    c->status = S4_CAL_FAILED;
    c->reason = reason;
    context->is_calibrated = false;
    context->calib_count = 0u;
    context->calib_sum = (Vec3f){0};
}
void terrain_calibration_begin(S4Calibration *c, TerrainContext *context,
                               uint32_t id, uint32_t deadline, uint32_t now)
{
    *c = (S4Calibration){.request_id = id,
                         .deadline_ms = deadline,
                         .sampled_ms = now,
                         .status = S4_CAL_RUNNING};
    context->is_calibrated = false;
    context->calib_count = 0u;
    context->calib_sum = (Vec3f){0};
    context->accel_offset = (Vec3f){0};
    context->filtered_accel = (Vec3f){0, 0, 1};
    terrain_algo_invalidate_window(context);
}
void terrain_calibration_step(S4Calibration *c, TerrainContext *context,
                              const Gy511RawAxes *raw, Gy511Result result,
                              bool stationary, uint32_t now)
{
    if (c->status != S4_CAL_RUNNING)
    {
        return;
    }
    if (robot_due(now, c->deadline_ms))
    {
        fail(c, context, S4_CAL_TIMEOUT);
    }
    else if (!stationary)
    {
        fail(c, context, S4_CAL_MOVING);
    }
    else if (result == GY511_ERROR || now - c->sampled_ms > 60u)
    {
        fail(c, context, S4_CAL_SENSOR);
    }
    else if (result == GY511_NEW && now != c->sampled_ms && raw != NULL)
    {
        c->sampled_ms = now;
        if (terrain_algo_calibrate_sample(context, raw))
        {
            c->status = S4_CAL_DONE;
        }
        else if (context->calib_count == 0u)
        {
            fail(c, context, S4_CAL_GRAVITY);
        }
    }
}
void terrain_calibration_cancel(S4Calibration *c, TerrainContext *context)
{
    fail(c, context, S4_CAL_STOP);
    c->status = S4_CAL_CANCELLED;
}
