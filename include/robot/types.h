#ifndef ROBOT_TYPES_H
#define ROBOT_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { COMMAND_NONE, COMMAND_START, COMMAND_STOP } RobotCommand;
typedef enum { NAV_NONE, NAV_LEFT, NAV_RIGHT, NAV_STRAIGHT, NAV_STOP } Navigation;
typedef enum { STATE_IDLE, STATE_FOLLOW_LINE, STATE_STOPPED, STATE_FAULT } MissionState;

typedef struct {
    bool valid;
    uint32_t sampled_ms;
} SampleStatus;

typedef struct {
    SampleStatus status;
    bool detected;
    float error; /* -1 = line left, 0 = centred, +1 = line right */
    bool junction;
    Navigation navigation; /* One-shot decoded event; NAV_NONE on next update. */
} LineReading;

typedef struct {
    SampleStatus status;
    bool collision;
    bool hump_detected;
    float pitch_deg;
    float yaw_rate_dps;
    bool height_valid;
    float hump_height_m; /* Estimate, meaningful only when height_valid. */
} TerrainReading;

typedef struct {
    SampleStatus status;
    float front_distance_m; /* Forward clearance, not an arbitrary servo angle. */
    bool profile_valid;
    float obstacle_width_m;
    float left_clearance_m;
    float right_clearance_m;
} ObstacleReading;

typedef struct {
    LineReading line;
    TerrainReading terrain;
    ObstacleReading obstacle;
} SensorFrame;

typedef struct {
    float left_mps;
    float right_mps;
} MotionTarget;

typedef struct {
    SampleStatus status;
    float left_mps;
    float right_mps;
    float distance_m;
} MotionFeedback;

typedef struct {
    MissionState state;
    MotionTarget target;
    const char *reason; /* Static string; no caller allocation. */
} VehicleController;

#endif
