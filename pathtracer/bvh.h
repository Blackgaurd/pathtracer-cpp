#pragma once

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

#include "aabb.h"
#include "linalg.h"
#include "object.h"

using ObjectPtr = std::shared_ptr<Object>;

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

    cur->aabb = objects[0]->aabb();
    for (size_t i = 1; i < objects.size(); i++) cur->aabb = cur->aabb.merge(objects[i]->aabb());

    int best_axis = -1;
    float split_pos = 0, min_cost = std::numeric_limits<float>::max();
    for (int axis = 0; axis < 3; axis++) {
        for (const ObjectPtr& object : objects) {
            float pos = object->centroid()[axis];
            int left_cnt = 0, right_cnt = 0;
            AABB left_aabb, right_aabb;
            for (const ObjectPtr& other : objects) {
                if (other->centroid()[axis] < pos) {
                    left_cnt++;
                    left_aabb = left_aabb.merge(other->aabb());
                } else {
                    right_cnt++;
                    right_aabb = right_aabb.merge(other->aabb());
                }
            }
            float cost = left_cnt * left_aabb.area() + right_cnt * right_aabb.area();
            std::cout << "cost " << cost << " left cnt " << left_cnt << " left area "
                      << left_aabb.area() << " right cnt " << right_cnt << " right area "
                      << right_aabb.area() << std::endl;
            if (cost > 0 && cost < min_cost) {
                min_cost = cost;
                best_axis = axis;
                split_pos = pos;
            }
        }
    }

    float nosplit_cost = objects.size() * cur->aabb.area();
    if (best_axis == -1 || min_cost > nosplit_cost) {
        std::cout << "returning on bad split " << best_axis << " " << min_cost << " "
                  << nosplit_cost << std::endl;
        cur->is_leaf = true;
        cur->objects = objects;
        return cur;
    }

    std::vector<ObjectPtr> left_objects, right_objects;
    for (const ObjectPtr& object : objects) {
        if (object->centroid()[best_axis] < split_pos)
            left_objects.push_back(object);
        else
            right_objects.push_back(object);
    }

    cur->is_leaf = false;
    cur->left = build_bvh(left_objects);
    cur->right = build_bvh(right_objects);
    return cur;
}
ObjectPtr intersect_bvh(const BVHNodePtr& node, const vec3& ray_o, const vec3& ray_d, float& t,
                        const ObjectPtr& ignore = nullptr) {
    // todo: there is a bug here, to be fixed
    if (!node) return nullptr;
    if (!node->aabb.intersect(ray_o, ray_d)) return nullptr;
    if (node->is_leaf) {
        ObjectPtr ret = nullptr;
        for (const ObjectPtr& object : node->objects) {
            if (object == ignore) continue;
            float t_ = std::numeric_limits<float>::max();
            if (object->intersect(ray_o, ray_d, t_) && t_ < t) {
                t = t_;
                ret = object;
            }
        }
        return ret;
    } else {
        float t_left = std::numeric_limits<float>::max();
        float t_right = std::numeric_limits<float>::max();
        ObjectPtr left = intersect_bvh(node->left, ray_o, ray_d, t_left, ignore);
        ObjectPtr right = intersect_bvh(node->right, ray_o, ray_d, t_right, ignore);
        if (left && right) {
            if (t_left < t_right) {
                t = t_left;
                return left;
            } else {
                t = t_right;
                return right;
            }
        }
        return left ? left : right;
    }
}

void print_bvh(const BVHNodePtr& node, int depth = 0) {
    if (!node) return;
    for (int i = 0; i < depth - 1; i++) std::cout << " | ";
    if (depth > 0) std::cout << " +-";
    std::cout << node->aabb.lb << " " << node->aabb.rt;
    if (node->is_leaf) std::cout << " leaf";
    std::cout << std::endl;
    print_bvh(node->left, depth + 1);
    print_bvh(node->right, depth + 1);
}
