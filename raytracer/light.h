#pragma once

#include <cmath>

#include "linalg.h"
#include "rng.h"

struct light_t {
    vec3 color;
    float intensity;
    bool soft_shadows;

    virtual ~light_t() = default;
    virtual vec3 absolute_dir(const vec3& p) const = 0;
    virtual vec3 random_dir(const vec3& p) const = 0;
    virtual float distance(const vec3& p) const = 0;
};

struct dir_light_t : public light_t {
    vec3 dir;

    dir_light_t() = default;
    dir_light_t(const vec3& dir, const vec3& color, float intensity) : dir(dir) {
        this->color = color;
        this->intensity = intensity;
        this->soft_shadows = false;
    }
    vec3 absolute_dir(const vec3& p) const override {
        return -dir;
    }
    float distance(const vec3& p) const override {
        return 1;  // to not mess with attenuation
    }
};

struct point_light_t : public light_t {
    vec3 pos;
    float radius;

    point_light_t() = default;
    point_light_t(const vec3& pos, float radius, const vec3& color, float intensity) : pos(pos), radius(radius) {
        this->color = color;
        this->intensity = intensity;
        this->soft_shadows = true;
    }
    vec3 absolute_dir(const vec3& p) const override {
        return (pos - p).normalize();
    }
    vec3 random_dir(const vec3& p) const override {
        // get a random point in a sphere
        vec3 rand_point = vec3(rng.rand01(), rng.rand01(), rng.rand01()).normalize();
        vec3 sphere_point = rand_point * radius * pow(rng.rand01(), 1.0f / 3.0f);
        return (sphere_point + pos - p).normalize();
    }
    float distance(const vec3& p) const override {
        return (pos - p).length();
    }
};