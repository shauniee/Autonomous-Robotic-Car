#ifndef ROBOT_SUBSYSTEMS_H
#define ROBOT_SUBSYSTEMS_H

#include "robot/types.h"

/* All update calls must return promptly; never wait for a full scan or network. */

/* Buddy 1: Pico W WiFi/MQTT or USB-only desktop backend.
   Call init once, then poll frequently on the same core (never from an ISR).
   Network operations do not wait for connection completion. */
void comms_init(void);
RobotCommand comms_poll(uint32_t now_ms);
void comms_publish(const VehicleController *controller, const SensorFrame *sensors,
                   const MotionFeedback *motion, uint32_t now_ms);

/* Buddy 2: this module alone may write motor outputs. */
void motion_init(void);
void motion_update(MotionTarget target, uint32_t now_ms);
MotionFeedback motion_read(void);
void motion_stop(void);

/* Buddy 3 */
void line_barcode_init(void);
LineReading line_barcode_update(uint32_t now_ms);

/* Buddy 4 */
void terrain_init(void);
TerrainReading terrain_update(uint32_t now_ms);

/* Buddy 5 */
void obstacle_init(void);
ObstacleReading obstacle_update(uint32_t now_ms);

#endif
