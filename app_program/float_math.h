#ifndef ROBOT_FLOAT_MATH_H
#define ROBOT_FLOAT_MATH_H
/* Bounded application calculations need no general libm dependency in the
   freestanding kernel build. Validate finite inputs before these helpers. */
static inline float robot_float_abs(float value)
{
    return value < 0.0f ? -value : value;
}
static inline float robot_float_min(float left, float right)
{
    return left < right ? left : right;
}
static inline float robot_float_max(float left, float right)
{
    return left > right ? left : right;
}
static inline float robot_float_signed(float magnitude, float sign)
{
    return sign < 0.0f ? -robot_float_abs(magnitude) :
        robot_float_abs(magnitude);
}
#endif
