#pragma once

#include <cmath>
#include <memory>

#include "aabb.h"
#include "linalg.h"
#include "material.h"

struct Object {
    MaterialPtr material;

    virtual ~Object() = default;
    virtual bool intersect(const vec3& ray_o, const vec3& ray_d, float& t) const = 0;
    virtual vec3 normal(const vec3& ray_d, const vec3& p) const = 0;
    virtual vec3 centroid() const = 0;
    virtual AABB aabb() const = 0;
};

using ObjectPtr = std::shared_ptr<Object>;

struct Sphere : public Object {
    vec3 center;
    float radius;

    Sphere() = default;
    Sphere(const vec3& center, float radius, const MaterialPtr& material)
        : center(center), radius(radius) {
        this->material = material;
    }

    bool intersect(const vec3& ray_o, const vec3& ray_d, float& t) const override {
        // float a = ray_d.dot(ray_d);
        float a = 1;  // assuming ray_d is normalized
        float b = 2 * ray_d.dot(ray_o - center);
        float c = ray_o.dot(ray_o) + center.dot(center) - 2 * ray_o.dot(center) - radius * radius;

        float d = b * b - 4 * a * c;
        if (d < 0) return false;

        t = (-b - std::sqrt(d)) / (2 * a);
        return t > 0;
    }
    vec3 normal(const vec3& ray_d, const vec3& p) const override {
        vec3 n = (p - center).normalize();
        return n.dot(ray_d) < 0 ? n : -n;
    }
    vec3 centroid() const override {
        return center;
    }
    AABB aabb() const override {
        return AABB(center - vec3(radius), center + vec3(radius));
    }
};

struct Triangle : public Object {
    vec3 v1, v2, v3;

    Triangle() = default;
    Triangle(const vec3& v1, const vec3& v2, const vec3& v3, const MaterialPtr& material)
        : v1(v1), v2(v2), v3(v3) {
        this->material = material;
    }

    bool intersect(const vec3& ray_o, const vec3& ray_d, float& t) const override {
        // EPS is defined in linalg.h

        vec3 edge1 = v2 - v1, edge2 = v3 - v1;
        vec3 h = ray_d.cross(edge2);
        float a = edge1.dot(h);
        if (std::abs(a) < EPS) return false;

        float f = 1 / a;
        vec3 s = ray_o - v1;
        float u = f * s.dot(h);
        if (u < 0 || u > 1) return false;

        vec3 q = s.cross(edge1);
        float v = f * ray_d.dot(q);
        if (v < 0 || u + v > 1) return false;

        t = f * edge2.dot(q);
        return t > EPS;
    }
    vec3 normal(const vec3& ray_d, const vec3& p) const override {
        vec3 edge1 = v2 - v1, edge2 = v3 - v1;
        vec3 n = edge1.cross(edge2).normalize();
        return n.dot(ray_d) < 0 ? n : -n;
    }
    vec3 centroid() const override {
        return (v1 + v2 + v3) / 3;
    }
    AABB aabb() const override {
        vec3 lb, rt;
        lb.x = std::min({v1.x, v2.x, v3.x});
        lb.y = std::min({v1.y, v2.y, v3.y});
        lb.z = std::min({v1.z, v2.z, v3.z});
        rt.x = std::max({v1.x, v2.x, v3.x});
        rt.y = std::max({v1.y, v2.y, v3.y});
        rt.z = std::max({v1.z, v2.z, v3.z});
        return AABB(lb, rt);
    }
};

// struct Quad : public Object {};