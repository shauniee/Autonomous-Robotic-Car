/* Cortex-M0+ compile-only layout probe for issue #109. */
#include "../../app_program/motion/motion.h"

#define PROBE(name, size) char probe_##name[(size)]
#define SIZE(type) PROBE(sizeof_##type, sizeof(type))
#define FIELD(type, field) PROBE(field_##type##__##field, sizeof(((type *)0)->field))

SIZE(MotionCommand);
FIELD(MotionCommand, sampled_ms);
FIELD(MotionCommand, target);
FIELD(MotionCommand, speed_action);
FIELD(MotionCommand, mode_steering);
SIZE(MotionHealth);
FIELD(MotionHealth, serviced_ms);
FIELD(MotionHealth, flags);
