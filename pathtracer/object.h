#pragma once

#include <memory>

#include "linalg.h"
#include "material.h"

struct Object {
    std::shared_ptr<Material> material;

    virtual ~Object() = default;
    virtual bool intersect(const vec3& ray_o, const vec3& ray_d,
                           float& t) const = 0;
    virtual vec3 normal(const vec3& p) const = 0;
};

struct Sphere : public Object {
    vec3 center;
    float radius;

    Sphere() = default;
    Sphere(const vec3& center, float radius,
           const std::shared_ptr<Material>& material)
        : center(center), radius(radius) {
        this->material = material;
    }

    bool intersect(const vec3& ray_o, const vec3& ray_d, float& t) const {
        // float a = ray_d.dot(ray_d);
        float a = 1;  // assuming ray_d is normalized
        float b = 2 * ray_d.dot(ray_o - center);
        float c = ray_o.dot(ray_o) + center.dot(center) -
                  2 * ray_o.dot(center) - radius * radius;

        float d = b * b - 4 * a * c;
        if (d < 0) return false;

        t = (-b - std::sqrt(d)) / (2 * a);
        return t > 0;
    }
    vec3 normal(const vec3& p) const {
        return (p - center).normalize();
    }
};