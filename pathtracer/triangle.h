#pragma once

#include "aabb.h"
#include "linalg.h"
#include "material.h"

struct Triangle {
    AABB aabb;
    vec3 centroid;
    vec3 v1, v2, v3;
    Material material;

    Triangle() = default;
    Triangle(const vec3& v1, const vec3& v2, const vec3& v3, const Material& material)
        : v1(v1), v2(v2), v3(v3) {
        this->material = material;
        this->centroid = (v1 + v2 + v3) / 3;

        this->aabb = AABB();
        this->aabb.merge(v1);
        this->aabb.merge(v2);
        this->aabb.merge(v3);
    }

    bool intersect(const vec3& ray_o, const vec3& ray_d, float& t) const {
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
        return t > 0;
    }
    vec3 normal(const vec3& ray_d, const vec3& p) const {
        vec3 edge1 = v2 - v1, edge2 = v3 - v1;
        vec3 n = edge1.cross(edge2).normalize();
        return n.dot(ray_d) < 0 ? n : -n;
    }
};
