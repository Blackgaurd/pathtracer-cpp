#pragma once

#include <fstream>
#include <iomanip>
#include <memory>
#include <ostream>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "bvh.h"
#include "camera.h"
#include "image.h"
#include "linalg.h"
#include "material.h"
#include "object.h"

#define FLOAT_INF 1e20

std::string strip(const std::string& s, char c) {
    size_t start = 0, end = s.size() - 1;
    while (start < s.size() && s[start] == c) start++;
    while (end > start && s[end] == c) end--;
    return s.substr(start, end - start + 1);
}
std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}
struct Scene {
    std::vector<ObjectPtr> objects;
    BVHNodePtr bvh_root;

    float shift_bias = 1e-4;

    Scene() = default;

    template <typename T, typename... Args>
    void add_object(Args&&... args) {
        objects.push_back(std::make_shared<T>(std::forward<Args>(args)...));
    }
    void load_obj(const std::string& filename,
                  const std::unordered_map<std::string, MaterialPtr>& materials) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file " << filename << std::endl;
            return;
        }

        std::string command, cur_mat;
        std::vector<vec3> vertices;
        while (file >> command) {
            if (command == "#") {
                // comment
                std::getline(file, command);
            } else if (command == "v") {
                float x, y, z;
                file >> x >> y >> z;
                vertices.emplace_back(x, y, z);
            } else if (command == "f") {
                if (materials.find(cur_mat) == materials.end()) {
                    std::cerr << "Material " << cur_mat << " not found" << std::endl;
                    break;
                }
                std::string args;
                std::getline(file, args);
                args = strip(args, ' ');
                std::vector<std::string> tokens = split(args, " ");
                std::vector<vec3> face_vertices;
                for (const std::string& token : tokens) {
                    std::vector<std::string> indices = split(token, "//");
                    int index = std::stoi(indices[0]) - 1;
                    face_vertices.push_back(vertices[index]);
                }
                if (tokens.size() == 3) {
                    add_object<Triangle>(face_vertices[0], face_vertices[1], face_vertices[2],
                                         materials.at(cur_mat));
                } else if (tokens.size() == 4) {
                    add_object<Triangle>(face_vertices[0], face_vertices[1], face_vertices[2],
                                         materials.at(cur_mat));
                    add_object<Triangle>(face_vertices[0], face_vertices[2], face_vertices[3],
                                         materials.at(cur_mat));
                }
            } else if (command == "usemtl") {
                file >> cur_mat;
            } else {
                std::getline(file, command);
            }
        }
        file.close();
    }

    void render(const Camera& camera, Image& image, int depth, int samples) {
        std::cout << "Building BVH..." << std::endl;
        bvh_root = build_bvh(objects);
        std::cout << "Done" << std::endl;

        std::cout << std::fixed << std::setprecision(2);
        Resolution res = image.res;
        for (int h = 0; h < res.height; h++) {
            float height_p = (float)h / res.height * 100;
            std::cout << "\rRendering: " << height_p << "%" << std::flush;
            for (int w = 0; w < res.width; w++) {
                vec3 ray_o, ray_d;
                vec3 color = {0, 0, 0};
                for (int s = 0; s < samples; s++) {
                    camera.get_ray(w, h, ray_o, ray_d);
                    color += path_trace(ray_o, ray_d, depth);
                }
                image.set_pixel(w, h, color / samples);
            }
        }
    }
    void run_thread(const Camera& camera, Image& image, int depth, int samples) {
        Resolution res = image.res;
        vec3 ray_o, ray_d;
        for (int h = 0; h < res.height; h++) {
            for (int w = 0; w < res.width; w++) {
                vec3 color = {0, 0, 0};
                for (int s = 0; s < samples; s++) {
                    camera.get_ray(w, h, ray_o, ray_d);
                    color += path_trace(ray_o, ray_d, depth);
                }
                vec3 prev_color = image.get_pixel(w, h);
                image.set_pixel(w, h, prev_color + color);
            }
        }
    }
    void render_threaded(const Camera& camera, Image& image, int depth, int samples, int threads) {
        // render on multiple threads
        int max_threads = std::thread::hardware_concurrency();
        if (threads > max_threads - 1) {
            throw std::runtime_error("more than " + std::to_string(max_threads - 1) +
                                     " threads not supported");
        }

        std::cout << "Building BVH..." << std::endl;
        bvh_root = build_bvh(objects);
        std::cout << "Done" << std::endl;

        std::vector<std::thread> pool(threads);
        std::vector<Image> images(threads, Image(image.res));
        std::vector<int> job_count(threads, samples / threads);
        for (int i = 0; i < samples % threads; i++) {
            job_count[i]++;
        }

        for (int i = 0; i < threads; i++) {
            pool[i] = std::thread(&Scene::run_thread, this, std::ref(camera), std::ref(images[i]),
                                  depth, job_count[i]);
        }
        for (int i = 0; i < threads; i++) pool[i].join();
        for (const Image& img : images) image.add_layer(img);
        image.divide(samples);
    }
    ObjectPtr intersect(const vec3& ray_o, const vec3& ray_d, float& hit_t,
                        const ObjectPtr& ignore = nullptr) {
        // brute force intersection
        ObjectPtr hit_obj = nullptr;
        hit_t = FLOAT_INF;
        for (ObjectPtr& obj : objects) {
            if (obj == ignore) continue;

            float t;
            if (obj->intersect(ray_o, ray_d, t) && t < hit_t) {
                hit_t = t;
                hit_obj = obj;
            }
        }
        return hit_obj;
    }
    vec3 path_trace(const vec3& ray_o, const vec3& ray_d, int depth) {
        if (depth == 0) return {0, 0, 0};

        float hit_t;
        ObjectPtr hit_obj = intersect_bvh(bvh_root, ray_o, ray_d, hit_t);
        // ObjectPtr hit_obj = intersect(ray_o, ray_d, hit_t);
        if (!hit_obj) return {0, 0, 0};

        if (hit_obj->material->type == EMIT) {
            return hit_obj->material->emission;
        }

        vec3 hit_p = ray_o + ray_d * hit_t;
        vec3 hit_n = hit_obj->normal(ray_d, hit_p);
        vec3 bias = hit_n * shift_bias;

        vec3 new_d = hit_obj->material->reflected_dir(ray_d, hit_n);
        vec3 new_o = hit_p + bias;

        vec3 rec_color = path_trace(new_o, new_d, depth - 1);
        vec3 emission = hit_obj->material->emission;
        vec3 surface_color = hit_obj->material->color;
        float theta = hit_n.dot(new_d);

        // idk where the 2 comes from
        return emission + 2 * rec_color * surface_color * theta;
    }
};