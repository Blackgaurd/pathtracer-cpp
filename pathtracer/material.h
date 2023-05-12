#pragma once

#include "linalg.h"
#include "rng.h"

vec3 hemisphere_sample(const vec3 &ray_d, const vec3 &normal) {
    // random hemisphere sample
    float u = rng.rand01(), v = rng.rand01();
    float theta = std::acos(2 * u - 1) - M_PI_2;
    float phi = 2 * M_PI * v;
    vec3 sample = {std::cos(theta) * std::cos(phi), std::cos(theta) * std::sin(phi),
                    std::sin(theta)};
    return sample.dot(normal) < 0 ? -sample : sample;
}
vec3 specular_sample(const vec3 &ray_d, const vec3 &normal, float roughness) {
    vec3 reflected = ray_d - 2 * ray_d.dot(normal) * normal;

    // rejection sampling, idk how to do this better
    vec3 ret;
    do {
        vec3 jitter = (vec3(rng.rand01(), rng.rand01(), rng.rand01()) - 0.5) * roughness;
        ret = reflected + jitter;
    } while (ret.dot(normal) < 0);
    return ret.normalize();
}

struct Material {
    enum Type {
        DIFFUSE,
        EMIT,
        SPECULAR,
    } type;
    vec3 color;
    vec3 emit_color;
    float roughness;

    Material() = default;
    Material(Type type, const vec3& color, const vec3& emit_color, float roughness)
        : type(type), color(color), emit_color(emit_color), roughness(roughness) {}
    vec3 reflected_dir(const vec3& ray_d, const vec3& normal) const {
        switch (type) {
            case DIFFUSE:
                return hemisphere_sample(ray_d, normal);
            case EMIT:
                return {0, 0, 0};
            case SPECULAR:
                return specular_sample(ray_d, normal, roughness);
            default:
                return hemisphere_sample(ray_d, normal);
        }
    }
};
