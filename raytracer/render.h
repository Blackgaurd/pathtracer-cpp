#pragma once

#include <cmath>
#include <fstream>
#include <memory>
#include <ostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "image.h"
#include "light.h"
#include "linalg.h"
#include "object.h"

#define BIAS 1e-5
#define RENDER_DEPTH 2
#define SHADOW_SAMPLES 16

// #define INDIRECT_LIGHTING
#define INDIRECT_SAMPLES 16

vec3 hemisphere_sample(const vec3& normal) {
    float u = rng.rand01(), v = rng.rand01();
    float theta = std::acos(2 * u - 1) - M_PI / 2;
    float phi = 2 * M_PI * v;
    vec3 sample = {std::cos(theta) * std::cos(phi),
                   std::cos(theta) * std::sin(phi), std::sin(theta)};
    return sample.dot(normal) < 0 ? -sample : sample;
}
std::shared_ptr<object_t> intersect(
    const vec3& ray_d, const vec3& ray_o,
    const std::vector<std::shared_ptr<object_t>>& objects,
    const std::shared_ptr<object_t> ignore, float& t) {
    t = std::numeric_limits<float>::max() / 2;
    std::shared_ptr<object_t> ret = nullptr;
    for (auto& object : objects) {
        if (object == ignore) continue;
        float t_candidate;
        if (object->intersect(ray_o, ray_d, t_candidate) && t_candidate < t) {
            t = t_candidate;
            ret = object;
        }
    }
    return ret;
}
float get_shadow(const vec3& ray_d, const vec3& ray_o,
                 const std::shared_ptr<light_t>& light,
                 const std::vector<std::shared_ptr<object_t>>& objects,
                 const std::shared_ptr<object_t> ignore) {
    float light_intensity = 0;
    float t;  // unused
    if (light->soft_shadows) {
        for (int i = 0; i < SHADOW_SAMPLES; i++) {
            vec3 rand_dir = light->random_dir(ray_o);
            auto obj = intersect(rand_dir, ray_o, objects, ignore, t);
            if (obj != nullptr) std::cout << "shadow" << std::endl;
            if (obj == nullptr) light_intensity += light->intensity;
        }
        return light_intensity / SHADOW_SAMPLES;
    } else {
        vec3 abs_dir = light->absolute_dir(ray_o);
        auto obj = intersect(abs_dir, ray_o, objects, ignore, t);
        return obj == nullptr ? light->intensity : 0;
    }
}
vec3 direct_lighting(const vec3& hit, const vec3& normal,
                     const std::shared_ptr<light_t>& light) {
    vec3 abs_dir = light->absolute_dir(hit);
    return light->color * std::max(0.0f, normal.dot(abs_dir)) / M_PI;
}
vec3 raycast(const vec3& ray_d, const vec3& ray_o,
             const std::vector<std::shared_ptr<object_t>>& objects,
             const std::vector<std::shared_ptr<light_t>>& lights,
             const vec3& bg_color, int depth) {
    if (depth == 0) return {0, 0, 0};

    // find first intersection
    float t;
    auto closest_object = intersect(ray_d, ray_o, objects, nullptr, t);
    if (closest_object == nullptr) return bg_color;

    vec3 hit = ray_o + ray_d * t;
    vec3 normal = closest_object->normal(hit);
    vec3 bias = normal * BIAS;

    vec3 color = {0, 0, 0};
    for (auto& light : lights) {
        float light_intensity =
            get_shadow(ray_d, ray_o, light, objects, closest_object);
        vec3 direct_color = direct_lighting(hit + bias, normal, light);
        vec3 indirect_color = {0, 0, 0};
#ifdef INDIRECT_LIGHTING
        for (int i = 0; i < INDIRECT_SAMPLES; i++) {
            vec3 indirect_dir = hemisphere_sample(normal);
            indirect_color += raycast(indirect_dir, hit + bias, objects, lights,
                                      bg_color, depth - 1);
        }
        indirect_color /= INDIRECT_SAMPLES;
#endif
        color += closest_object->color *
                 (direct_color * light_intensity + indirect_color);
    }
    return color;
}
void render(float fov_rad, const vec3& look_from, const vec3& look_at,
            const vec3& up, float image_distance, const vec3& bg_color,
            const std::vector<std::shared_ptr<object_t>>& objects,
            const std::vector<std::shared_ptr<light_t>>& lights,
            image_t& image) {
    std::pair<int, int> res = {image.height, image.width};

#define height first
#define width second
    std::pair<float, float> v_res = {
        2.0 * image_distance * std::tan(fov_rad / 2) *
            static_cast<float>(res.height) / res.width,
        2.0 * image_distance * std::tan(fov_rad / 2)};
    float cell_width = v_res.width / res.width;

    mat4 camera = mat4_constructors::camera(look_from, look_at, up);
    for (int h = 0; h < res.height; h++) {
        for (int w = 0; w < res.width; w++) {
            vec3 ray_d = {w * cell_width - v_res.width / 2 + cell_width / 2,
                          h * cell_width - v_res.height / 2 + cell_width / 2,
                          -image_distance};
            ray_d = camera.transform_dir(ray_d).normalize();

            // abs
            vec3 color = raycast(ray_d, look_from, objects, lights, bg_color,
                                 RENDER_DEPTH);
            // std::cout << color << std::endl;
            image.set_pixel(w, h, color);
            // break;
        }
    }
#undef height
#undef width
}