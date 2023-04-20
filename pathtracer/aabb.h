#pragma once

#include <algorithm>

#include "linalg.h"

// axis aligned bounding box
struct AABB {
    vec3 lb, rt;

    AABB() = default;
    AABB(const vec3& lb, const vec3& rt) : lb(lb), rt(rt) {}

    AABB merge(const AABB& other) const {
        return AABB(
            vec3(std::min(lb.x, other.lb.x), std::min(lb.y, other.lb.y),
                 std::min(lb.z, other.lb.z)),
            vec3(std::max(rt.x, other.rt.x), std::max(rt.y, other.rt.y),
                 std::max(rt.z, other.rt.z)));
    }
    bool intersect(const vec3& ray_o, const vec3& ray_d) const {
        vec3 inv_d = 1 / ray_d;
        vec3 t1 = (lb - ray_o) * inv_d;
        vec3 t2 = (rt - ray_o) * inv_d;

        float tmin = std::max(
            {std::min(t1.x, t2.x), std::min(t1.y, t2.y), std::min(t1.z, t2.z)});
        float tmax = std::min(
            {std::max(t1.x, t2.x), std::max(t1.y, t2.y), std::max(t1.z, t2.z)});

        if (tmax < 0) return false;
        return tmin <= tmax;
    }
    float surface_area() const {
        vec3 d = rt - lb;
        return 2 * (d.x * d.y + d.x * d.z + d.y * d.z);
    }
};