#pragma once

#include <cmath>
#include <random>

#include "linalg.h"

std::mt19937 rng;
std::uniform_real_distribution<float> dist(0, 1);

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

// todo: point light using light attenuation and inverse square law
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
        float angle = dist(rng) * 2 * M_PI;
        float radius_ = std::sqrt(dist(rng)) * radius;

        vec3 pos_dir = (pos - p).normalize();  // direction from point to light
        vec3 tangent = pos_dir.cross({0, 1, 0}).normalize();
        vec3 bitangent = tangent.cross(pos_dir).normalize();

        vec3 new_point = pos + tangent * radius_ * std::cos(angle) + bitangent * radius_ * std::sin(angle);
        return (new_point - p).normalize();
    }
    float distance(const vec3& p) const override {
        return (pos - p).length();
    }
};