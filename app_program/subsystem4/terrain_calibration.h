#ifndef TERRAIN_CALIBRATION_H
#define TERRAIN_CALIBRATION_H
#include "terrain_algo.h"
typedef enum
{
    S4_CAL_IDLE,
    S4_CAL_RUNNING,
    S4_CAL_DONE,
    S4_CAL_FAILED,
    S4_CAL_CANCELLED
} S4CalibrationStatus;
typedef enum
{
    S4_CAL_OK,
    S4_CAL_TIMEOUT,
    S4_CAL_MOVING,
    S4_CAL_SENSOR,
    S4_CAL_GRAVITY,
    S4_CAL_STOP
} S4CalibrationReason;
typedef struct
{
    uint32_t request_id, deadline_ms, sampled_ms;
    S4CalibrationStatus status;
    S4CalibrationReason reason;
} S4Calibration;
void terrain_calibration_begin(S4Calibration *cal, TerrainContext *context,
                               uint32_t id, uint32_t deadline, uint32_t now);
void terrain_calibration_step(S4Calibration *cal, TerrainContext *context,
                              const Gy511RawAxes *raw, Gy511Result result,
                              bool stationary, uint32_t now);
void terrain_calibration_cancel(S4Calibration *cal, TerrainContext *context);
#endif
