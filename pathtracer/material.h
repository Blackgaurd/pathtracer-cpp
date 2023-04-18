#pragma once

#include "linalg.h"
#include "rng.h"

enum MaterialType {
    DIFFUSE,
    EMIT,
    SPECULAR,
    MIRROR,
};

struct Material {
    MaterialType type;
    vec3 color;
    vec3 emission;

    virtual ~Material() = default;
    virtual vec3 reflected_dir(const vec3& ray_d, const vec3& normal) const = 0;
};

struct Diffuse : public Material {
    // lambertian diffuse

    Diffuse() = default;
    Diffuse(const vec3& color) {
        this->type = DIFFUSE;
        this->color = color;
        this->emission = {0, 0, 0};
    }

    vec3 reflected_dir(const vec3& ray_d, const vec3& normal) const override {
        // random hemisphere sample
        float u = rng.rand01(), v = rng.rand01();
        float theta = std::acos(2 * u - 1) - M_PI_2;
        float phi = 2 * M_PI * v;
        vec3 sample = {std::cos(theta) * std::cos(phi),
                       std::cos(theta) * std::sin(phi), std::sin(theta)};
        return sample.dot(normal) < 0 ? -sample : sample;
    }
};

struct Emit : public Material {
    // light source

    Emit() = default;
    Emit(const vec3& color) {
        this->type = EMIT;
        this->color = color;
        this->emission = color;
    }

    vec3 reflected_dir(const vec3& ray_d, const vec3& normal) const override {
        // no reflection
        return ray_d;
    }
};

/* struct Specular : public Material {
    // specular reflection
    float roughness;

    Specular() = default;
    Specular(const vec3& color, float roughness) : roughness(roughness) {
        this->type = SPECULAR;
        this->color = color;
        this->emission = {0, 0, 0};
    }
};

struct Mirror : public Material {
    // specular with 0 roughness

    Mirror() = default;
    Mirror(const vec3& color) {
        this->type = MIRROR;
        this->color = color;
        this->emission = {0, 0, 0};
    }
}; */