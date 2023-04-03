#pragma once

#include "linalg.h"

struct dir_light_t {
    vec3 dir;
    vec3 color;
    float intensity;

    dir_light_t() = default;
    dir_light_t(const vec3& dir, const vec3& color, float intensity)
        : dir(dir), color(color), intensity(intensity) {}
};