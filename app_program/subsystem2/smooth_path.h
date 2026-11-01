#ifndef SMOOTH_PATH_H
#define SMOOTH_PATH_H
#include <math.h>
/* Local geometry only; persistent references use Q8 mm in Motion. */
#define SMOOTH_PATH_C 0.605143689f
#define SMOOTH_PATH_PI 3.141592654f
static inline float smooth_path_length(float d, float chassis_left, int sign)
{
    return (d + sign * chassis_left) / SMOOTH_PATH_C;
}
static inline int smooth_path_valid(float length, float track)
{
    return isfinite(length) && isfinite(track) && track > 0.0f &&
        length >= 3.0f * SMOOTH_PATH_PI * track / 8.0f;
}
static inline void smooth_path_reference(float length, float track, int sign,
    float progress, float position[2],
    float ratio[2])
{
    float u = fminf(fmaxf(progress / length, 0.0f), 1.0f);
    float theta = sign * (SMOOTH_PATH_PI / 2.0f) * u * u * (3.0f - 2.0f * u);
    float curvature = sign * 3.0f * SMOOTH_PATH_PI * u * (1.0f - u) / length;
    /* Past the arc, continue tangent until Mission verifies the exit. */
    position[0] = progress - track * theta / 2.0f;
    position[1] = progress + track * theta / 2.0f;
    ratio[0] = 1.0f - track * curvature / 2.0f;
    ratio[1] = 1.0f + track * curvature / 2.0f;
}
#endif
