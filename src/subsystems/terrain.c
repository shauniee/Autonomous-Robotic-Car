#include "robot/subsystems.h"

void terrain_init(void) {
    /* TODO Buddy 4: initialise actual IMU, confirm axes, estimate bias. */
}

TerrainReading terrain_update(uint32_t now_ms) {
    (void)now_ms;
    /* TODO: orientation filtering, hump/collision events, turn rate.
       Validate height estimation against measured humps; keep height_valid
       false until an estimate is available. */
    return (TerrainReading){0};
}
