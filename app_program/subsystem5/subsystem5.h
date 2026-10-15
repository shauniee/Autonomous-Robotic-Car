#ifndef SUBSYSTEM5_H
#define SUBSYSTEM5_H
#include "../app.h"
#include <stdbool.h>
#include <stdint.h>
#define S5_PRI_OBSTACLE 17
#define S5_SCAN_CAPACITY 9u
typedef enum { SCAN_IDLE, SCAN_RUNNING, SCAN_DONE, SCAN_FAILED,
    SCAN_CANCELLED } ScanStatus;
typedef struct
{
    float angle_deg, range_m;
    uint32_t sampled_ms;
    bool valid;
} ObstacleRay;
typedef struct
{
    uint32_t front_ms, sequence, progress_ms, profile_ms, scan_id;
    float front_m, closest_m, width_m, left_clearance_m, right_clearance_m;
    float obstacle_forward_m, obstacle_left_m;
    bool front_valid, profile_valid, configured;
    ScanStatus scan_status;
    uint8_t ray_count;
    ObstacleRay rays[S5_SCAN_CAPACITY];
} ObstacleSnapshot;
ER subsystem5_init(void);
ER subsystem5_start(void);
ER s5_obstacle_copy(ObstacleSnapshot *out);
ER s5_scan_request(uint32_t scan_id, uint32_t deadline_ms);
void s5_scan_cancel(void);
bool s5_profile_build(ObstacleSnapshot *snapshot);
#endif
