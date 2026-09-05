#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

/* Demonstration values only. Measure and tune before enabling real motors. */
#define CONTROL_PERIOD_MS 20u
#define TELEMETRY_PERIOD_MS 500u
#define SENSOR_MAX_AGE_MS 250u
#define BASE_SPEED_MPS 0.15f
#define MAX_SPEED_MPS 0.25f
#define LINE_KP 0.10f
#define OBSTACLE_STOP_M 0.25f

#endif
