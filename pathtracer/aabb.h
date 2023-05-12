#pragma once

#include "linalg.h"

// axis aligned bounding box
struct AABB {
    vec3 lb = FLOAT_INF, rt = -FLOAT_INF;

    AABB() = default;
    AABB(const vec3& lb, const vec3& rt) : lb(lb), rt(rt) {}

    void merge(const AABB& other) {
        lb = component_min(lb, other.lb);
        rt = component_max(rt, other.rt);
    }
    void merge(const vec3& p) {
        lb = component_min(lb, p);
        rt = component_max(rt, p);
    }
    bool intersect_inv(const vec3& ray_o, const vec3& inv_ray_d) const {
        vec3 t1 = (lb - ray_o) * inv_ray_d;
        vec3 t2 = (rt - ray_o) * inv_ray_d;

        float tmax = std::min({std::max(t1.x, t2.x), std::max(t1.y, t2.y), std::max(t1.z, t2.z)});
        float tmin = std::max({std::min(t1.x, t2.x), std::min(t1.y, t2.y), std::min(t1.z, t2.z)});

        if (tmax < 0) return false;
        return tmin <= tmax;
    }
    bool intersect(const vec3& ray_o, const vec3& ray_d) const {
        vec3 inv_d = 1 / ray_d;
        return intersect_inv(ray_o, inv_d);
    }
    float area() const {
        // returns half of the surface area
        if (!is_valid()) return 0;
        vec3 d = rt - lb;
        return d.x * d.y + d.x * d.z + d.y * d.z;
    }
    bool is_valid() const {
        return lb.x <= rt.x && lb.y <= rt.y && lb.z <= rt.z;
    }

    friend std::ostream& operator<<(std::ostream& os, const AABB& aabb) {
        return os << "AABB: " << aabb.lb << " " << aabb.rt;
    }
};