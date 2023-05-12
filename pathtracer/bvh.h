#pragma once

#include <deque>
#include <iostream>

#include "aabb.h"
#include "linalg.h"
#include "material.h"
#include "tiny_obj_loader.h"
#include "triangle.h"

struct BVHNode {
    AABB aabb;
    int left, right;
    int tri_start, tri_end;

    BVHNode() = default;
    BVHNode(int left, int right, int tri_start, int tri_end) {
        this->left = left;
        this->right = right;
        this->tri_start = tri_start;
        this->tri_end = tri_end;
    }

    bool is_leaf() const {
        return left == -1 && right == -1;
    }
};

struct BVH {
    bool built = false;
    std::vector<Triangle> triangles;
    std::vector<int> tri_idx;
    std::vector<BVHNode> nodes;

    BVH() = default;

    void add_triangle(const Triangle& tri) {
        built = false;
        triangles.push_back(tri);
    }
    size_t size() const {
        return triangles.size();
    }
    bool empty() const {
        return triangles.empty();
    }
    void find_best_axis(BVHNode& cur, int& best_axis, float& split_pos, float& min_cost) const {
        best_axis = -1;
        split_pos = 0;
        min_cost = FLOAT_INF;
        for (int axis = 0; axis < 3; axis++) {
            for (int i = cur.tri_start; i <= cur.tri_end; i++) {
                const Triangle& tri = triangles[tri_idx[i]];
                float pos = tri.centroid[axis];
                int left_cnt = 0, right_cnt = 0;
                AABB left_aabb, right_aabb;
                for (int j = cur.tri_start; j <= cur.tri_end; j++) {
                    const Triangle& tri = triangles[tri_idx[j]];
                    if (tri.centroid[axis] < pos) {
                        left_cnt++;
                        left_aabb.merge(tri.aabb);
                    } else {
                        right_cnt++;
                        right_aabb.merge(tri.aabb);
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
    }
    void build() {
        // stack based building algorithm
        tri_idx.resize(triangles.size());
        for (int i = 0; i < int(tri_idx.size()); i++) tri_idx[i] = i;

        nodes.reserve(triangles.size() * 2);
        nodes.push_back(BVHNode(-1, -1, 0, triangles.size() - 1));

        std::deque<int> stack;
        stack.push_back(0);
        while (!stack.empty()) {
            BVHNode& cur = nodes[stack.back()];
            stack.pop_back();

            // build current aabb
            for (int i = cur.tri_start; i <= cur.tri_end; i++)
                cur.aabb.merge(triangles[tri_idx[i]].aabb);

            // find the best axis to split
            int best_axis;
            float split_pos, min_cost;
            find_best_axis(cur, best_axis, split_pos, min_cost);

            int tri_count = cur.tri_end - cur.tri_start + 1;
            float nosplit_cost = tri_count * cur.aabb.area();
            if (best_axis == -1 || min_cost > nosplit_cost) {
                // no split
                continue;
            }

            // triangle "sorting" algorithm:
            // two pointer method
            // "array of centroids" below
            // e.g. [9, 7, 3, 6, 3, 5, 2, 9, 4, 10] with split_pos = 7
            // should become [3, 6, 3, 5, 2, 4, 9, 7, 9, 10]
            //                      split here ^
            // - two pointers, one at the start, one at the end
            // - advance start pointer until element at index >= 7 (split_pos)
            // - move back end pointer until element at index < 7 (split_pos)
            // - swap the two elements
            // - repeat until start pointer >= end pointer
            // - count the number of elements on the left (left_cnt = 6)

            int start = cur.tri_start, end = cur.tri_end;
            int left_count = 0;
            while (start < end) {
                if (triangles[tri_idx[start]].centroid[best_axis] < split_pos) {
                    start++;
                    left_count++;
                } else if (triangles[tri_idx[end]].centroid[best_axis] >= split_pos) {
                    end--;
                } else {
                    std::swap(tri_idx[start], tri_idx[end]);
                }
            }

            if (left_count == 0 || left_count == tri_count) {
                // no split
                continue;
            }

            int left = nodes.size();
            int left_start = cur.tri_start, left_end = left_start + left_count - 1;
            nodes.push_back(BVHNode(-1, -1, left_start, left_end));
            cur.left = left;
            stack.push_back(left);

            int right = nodes.size();
            int right_start = cur.tri_start + left_count, right_end = cur.tri_end;
            nodes.push_back(BVHNode(-1, -1, right_start, right_end));
            cur.right = right;
            stack.push_back(right);
        }
        built = true;
    }
    int intersect(const vec3& ray_o, const vec3& ray_d, float& t) const {
        vec3 inv_ray_d = 1 / ray_d;
        std::deque<int> stack;
        stack.push_back(0);

        int ret = -1;
        t = FLOAT_INF;
        while (!stack.empty()) {
            const BVHNode& cur = nodes[stack.back()];
            stack.pop_back();
            if (!cur.aabb.intersect_inv(ray_o, inv_ray_d)) continue;
            if (cur.is_leaf()) {
                for (int i = cur.tri_start; i <= cur.tri_end; i++) {
                    float t_;
                    const Triangle& tri = triangles[tri_idx[i]];
                    if (tri.intersect(ray_o, ray_d, t_) && t_ < t) {
                        t = t_;
                        ret = tri_idx[i];
                    }
                }
            } else {
                stack.push_back(cur.left);
                stack.push_back(cur.right);
            }
        }

        return ret;
    }
    void load_obj(const std::string& filename, const std::string& mtl_path = "./") {
        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = mtl_path;

        tinyobj::ObjReader reader;
        if (!reader.ParseFromFile(filename, reader_config)) {
            if (!reader.Error().empty())
                throw std::runtime_error("TinyObjLoader: " + reader.Error());
            throw std::runtime_error("TinyObjLoader: unknown error");
        }

        if (!reader.Warning().empty()) {
            std::cerr << "TinyObjLoader: " << reader.Warning() << '\n';
        }

        const auto& attrib = reader.GetAttrib();
        const auto& shapes = reader.GetShapes();
        const auto& materials = reader.GetMaterials();

        for (const auto& shape : shapes) {
            size_t index_offset = 0;
            for (size_t poly = 0; poly < shape.mesh.num_face_vertices.size(); poly++) {
                size_t vertices = shape.mesh.num_face_vertices[poly];
                std::vector<vec3> points;
                for (size_t v = 0; v < vertices; v++) {
                    auto idx = shape.mesh.indices[index_offset + v];
                    auto vx = attrib.vertices[3 * idx.vertex_index];
                    auto vy = attrib.vertices[3 * idx.vertex_index + 1];
                    auto vz = attrib.vertices[3 * idx.vertex_index + 2];
                    points.emplace_back(vx, vy, vz);
                }
                index_offset += vertices;

                int mat_id = shape.mesh.material_ids[poly];
                auto& mat = materials[mat_id];
                Material material;
                switch (mat.illum) {
                    case 1: {
                        // diffuse
                        vec3 color = vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
                        material = Material(Material::DIFFUSE, color, 0, 0);
                        break;
                    }
                    case 2: {
                        // emit
                        vec3 color = vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
                        material = Material(Material::EMIT, 0, color, 0);
                        break;
                    }
                    default: {
                        std::cerr << "Unknown material type with illum: " << mat.illum << '\n';
                        std::cerr << "Using default material: Diffuse(0.5)" << '\n';
                        material = Material(Material::DIFFUSE, 0.5, 0, 0);
                    }
                }
                add_triangle(Triangle(points[0], points[1], points[2], material));
            }
        }
    }
    void print(int node_idx = 0, int depth = 0, std::string dir = "root") const {
        if (node_idx == -1) return;
        std::cout << node_idx << ":\t";
        for (int i = 0; i < depth; i++) std::cout << " | ";
        if (depth > 0) std::cout << " +-";

        const BVHNode& node = nodes[node_idx];
        std::cout << node.aabb.lb << ' ' << node.aabb.rt;
        if (node.is_leaf()) {
            std::cout << " leaf, tri: " << node.tri_start << " -> " << node.tri_end;
            std::cout << " (" << dir << ")\n";
        } else {
            std::cout << " tri: " << node.tri_start << " -> " << node.tri_end;
            std::cout << " (" << dir << ")\n";
            print(node.left, depth + 1, "left");
            print(node.right, depth + 1, "right");
        }
    }
};