#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "aabb.h"
#include "linalg.h"
#include "object.h"

struct BVHNode {
    AABB aabb;
    bool is_leaf;
    std::vector<ObjectPtr> objects;
    std::shared_ptr<BVHNode> left, right;
};
using BVHNodePtr = std::shared_ptr<BVHNode>;

BVHNodePtr build_bvh(const std::vector<ObjectPtr>& objects) {
    // n^2 :despair:
    BVHNodePtr cur = std::make_shared<BVHNode>();

    cur->aabb = objects[0]->aabb;
    for (size_t i = 1; i < objects.size(); i++) cur->aabb.merge(objects[i]->aabb);

    int best_axis = -1;
    float split_pos = 0, min_cost = 1e30;
    for (int axis = 0; axis < 3; axis++) {
        for (const ObjectPtr& object : objects) {
            float pos = object->centroid[axis];
            int left_cnt = 0, right_cnt = 0;
            AABB left_aabb, right_aabb;
            for (const ObjectPtr& other : objects) {
                if (other->centroid[axis] < pos) {
                    left_cnt++;
                    left_aabb.merge(other->aabb);
                } else {
                    right_cnt++;
                    right_aabb.merge(other->aabb);
                }
            }
            if (left_cnt == 0 || right_cnt == 0) continue;
            float cost = left_cnt * left_aabb.area() + right_cnt * right_aabb.area();
            if (cost < min_cost) {
                min_cost = cost;
                best_axis = axis;
                split_pos = pos;
            }
        }
    }

    float nosplit_cost = objects.size() * cur->aabb.area();
    if (best_axis == -1 || min_cost > nosplit_cost) {
        cur->is_leaf = true;
        cur->objects = objects;
        return cur;
    }

    std::vector<ObjectPtr> left_objects, right_objects;
    for (const ObjectPtr& object : objects) {
        if (object->centroid[best_axis] < split_pos)
            left_objects.push_back(object);
        else
            right_objects.push_back(object);
    }

    cur->is_leaf = false;
    cur->left = build_bvh(left_objects);
    cur->right = build_bvh(right_objects);
    return cur;
}
ObjectPtr intersect_bvh_wpr(const BVHNodePtr& node, const vec3& ray_o, const vec3& ray_d,
                            const vec3& inv_ray_d, float& t, const ObjectPtr& ignore) {
    if (!node->aabb.intersect_inv(ray_o, inv_ray_d)) return nullptr;
    if (node->is_leaf) {
        ObjectPtr ret = nullptr;
        float t_;
        for (const ObjectPtr& object : node->objects) {
            if (object == ignore) continue;
            if (object->intersect(ray_o, ray_d, t_) && t_ < t) {
                t = t_;
                ret = object;
            }
        }
        return ret;
    } else {
        float t_left = 1e30, t_right = 1e30;
        ObjectPtr left = intersect_bvh_wpr(node->left, ray_o, ray_d, inv_ray_d, t_left, ignore);
        ObjectPtr right = intersect_bvh_wpr(node->right, ray_o, ray_d, inv_ray_d, t_right, ignore);
        if (left && right) {
            if (t_left < t_right) {
                t = t_left;
                return left;
            } else {
                t = t_right;
                return right;
            }
        } else if (left) {
            t = t_left;
            return left;
        } else if (right) {
            t = t_right;
            return right;
        }
        return nullptr;
    }
}
ObjectPtr intersect_bvh(const BVHNodePtr& node, const vec3& ray_o, const vec3& ray_d, float& t,
                        const ObjectPtr& ignore = nullptr) {
    vec3 inv_d = 1 / ray_d;
    return intersect_bvh_wpr(node, ray_o, ray_d, inv_d, t, ignore);
}
void print_bvh(const BVHNodePtr& node, std::string dir = "root", int depth = 0) {
    if (!node) return;
    for (int i = 0; i < depth - 1; i++) std::cout << " | ";
    if (depth > 0) std::cout << " +-";
    std::cout << node->aabb.lb << " " << node->aabb.rt;
    std::cout << " " << dir;
    if (node->is_leaf) std::cout << " leaf";
    std::cout << std::endl;
    print_bvh(node->left, "left", depth + 1);
    print_bvh(node->right, "right", depth + 1);
}
