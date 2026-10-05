#ifndef ROBOT_RIGHT_IR_FOLLOW_H
#define ROBOT_RIGHT_IR_FOLLOW_H
#include <stdbool.h>
#include <stdint.h>
#define FOLLOW_IR_GPIO 27u
#define FOLLOW_IR_ADC_CHANNEL 1u
#define FOLLOW_MIN_CONTRAST 300
#define FOLLOW_BASE_DUTY .22f
#define FOLLOW_MAX_DUTY .35f
#define FOLLOW_KP .30f
#define FOLLOW_EDGE_TIMEOUT_MS 1500u
/* Right edge: more black under the sensor -> steer right. */
#define FOLLOW_EDGE_SIGN 1
#define FOLLOW_LEFT_FORWARD_A 1
#define FOLLOW_RIGHT_FORWARD_A 1
typedef struct { bool valid; float fraction, error, left, right; } FollowResult;
FollowResult right_ir_steering(uint16_t raw,uint16_t black,uint16_t white,bool calibrated,int edge);
#endif
