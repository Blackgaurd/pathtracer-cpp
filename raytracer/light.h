#pragma once

#include "linalg.h"

struct light_t {
    vec3 color;
    float intensity;

    virtual ~light_t() = default;
    virtual vec3 get_direction(const vec3& p) const = 0;
};

struct dir_light_t : public light_t {
    vec3 dir;

    dir_light_t() = default;
    dir_light_t(const vec3& dir, const vec3& color, float intensity) : dir(dir) {
        this->color = color;
        this->intensity = intensity;
    }
    vec3 get_direction(const vec3& p) const override { return -dir; }
};