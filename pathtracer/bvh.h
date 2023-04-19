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
    ObjectPtr object;
    std::shared_ptr<BVHNode> left, right;
};
using BVHNodePtr = std::shared_ptr<BVHNode>;

BVHNodePtr build_bvh(std::vector<ObjectPtr>& objects) {
    BVHNodePtr cur = std::make_shared<BVHNode>();
    if (objects.size() == 1) {
        cur->is_leaf = true;
        cur->object = objects[0];
        cur->aabb = objects[0]->aabb();
        return cur;
    }

    cur->is_leaf = false;

    // calculate the bounding box of all objects
    cur->aabb = objects[0]->aabb();
    for (const ObjectPtr& obj : objects) {
        cur->aabb = cur->aabb.merge(obj->aabb());
    }

    // find the best axis and split point
    // for now:
    // split along the longest axis such that
    // there are an equal number of objects on each side
    vec3 diag = cur->aabb.rt - cur->aabb.lb;
    int axis = 2;
    if (diag.x > diag.y && diag.x > diag.z)
        axis = 0;
    else if (diag.y > diag.z)
        axis = 1;

    std::sort(objects.begin(), objects.end(),
              [axis](const ObjectPtr& a, const ObjectPtr& b) {
                  return a->aabb().lb[axis] < b->aabb().lb[axis];
              });
    std::vector<ObjectPtr> left, right;
    for (size_t i = 0; i < objects.size(); i++) {
        if (i < objects.size() / 2)
            left.push_back(objects[i]);
        else
            right.push_back(objects[i]);
    }
    cur->left = build_bvh(left);
    cur->right = build_bvh(right);

    return cur;
}
ObjectPtr intersect(const BVHNodePtr& node, const vec3& ray_o,
                    const vec3& ray_d, float& t) {
    if (!node->aabb.intersect(ray_o, ray_d)) return nullptr;
    if (node->is_leaf) {
        if (node->object->intersect(ray_o, ray_d, t)) return node->object;
        return nullptr;
    }
    ObjectPtr left = intersect(node->left, ray_o, ray_d, t);
    ObjectPtr right = intersect(node->right, ray_o, ray_d, t);
    if (left) return left;
    return right;
}
