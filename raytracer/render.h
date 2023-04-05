#pragma once

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "image.h"
#include "light.h"
#include "linalg.h"
#include "object.h"

#define BIAS 1e-4
#define RENDER_DEPTH 2
#define SHADOW_SAMPLES 32
#define INDIRECT_SAMPLES 32

vec3 hemisphere_sample(const vec3& normal) {
    float u = rng.rand01(), v = rng.rand01();
    float theta = std::acos(2 * u - 1) - M_PI / 2;
    float phi = 2 * M_PI * v;
    vec3 sample = {
        std::cos(theta) * std::cos(phi),
        std::cos(theta) * std::sin(phi),
        std::sin(theta)};
    return sample.dot(normal) < 0 ? -sample : sample;
}
bool intersect(const vec3& ray_d, const vec3& ray_o, const std::vector<std::shared_ptr<object_t>>& objects, const std::shared_ptr<object_t> ignore, std::shared_ptr<object_t> out, float& t) {
    t = std::numeric_limits<float>::max() / 2;
    out = nullptr;
    for (auto& object : objects) {
        if (object == ignore) continue;
        float t_candidate;
        if (object->intersect(ray_o, ray_d, t_candidate) && t_candidate < t) {
            t = t_candidate;
            out = object;
        }
    }
    return out != nullptr;
}
vec3 raycast(const vec3& ray_d, const vec3& ray_o, const std::vector<std::shared_ptr<object_t>>& objects, const std::vector<std::shared_ptr<light_t>>& lights, const vec3& bg_color, int depth) {
    if (depth == 0) return {0, 0, 0};

    // check intersections
    float t;
    std::shared_ptr<object_t> closest_object = nullptr;
    if (!intersect(ray_d, ray_o, objects, nullptr, closest_object, t)) return bg_color;

    // direct lighting
    vec3 hit = ray_o + ray_d * t;
    vec3 normal = closest_object->normal(hit);
    vec3 bias = normal * BIAS;

    // shadows
    float shadow_intensity = 0;
    for (auto& light : lights) {
        if (light->soft_shadows) {
            for (int i = 0; i < SHADOW_SAMPLES; i++) {
                vec3 rand_dir = light->random_dir(hit);
                bool in_shadow = intersect(rand_dir, hit + bias, objects, closest_object, nullptr, t);
                if (!in_shadow) shadow_intensity += 1.0f;
            }
            shadow_intensity /= SHADOW_SAMPLES;
        } else {
            vec3 abs_dir = light->absolute_dir(hit);
            bool in_shadow = intersect(abs_dir, hit + bias, objects, closest_object, nullptr, t);
            if (!in_shadow) shadow_intensity = 1.0f;
        }
    }

    // direct lighting
    vec3 direct_color = {0, 0, 0};
    for (auto& light : lights) {
        vec3 abs_dir = light->absolute_dir(hit);
        direct_color += closest_object->color * light->color * std::max(0.0f, normal.dot(abs_dir));
    }
    direct_color /= M_PI;

    // indirect lighting
    vec3 indirect_color = {0, 0, 0};
    for (int i = 0; i < INDIRECT_SAMPLES; i++) {
        vec3 indirect_dir = hemisphere_sample(normal);
        indirect_color += raycast(indirect_dir, hit + bias, objects, lights, bg_color, depth - 1);
    }
    indirect_color /= INDIRECT_SAMPLES;

    return direct_color * shadow_intensity + indirect_color * (1 - shadow_intensity);

    /* vec3 direct_color = {0, 0, 0};
    for (auto& light : lights) {
        // shadows
        vec3 abs_dir = light->absolute_dir(hit);
        float shadow_intensity = 0;
        if (light->soft_shadows) {
            for (int i = 0; i < SHADOW_SAMPLES; i++) {
                vec3 rand_dir = light->random_dir(hit);
                bool in_shadow = intersect(rand_dir, hit + bias, objects, closest_object, nullptr, t);
                if (!in_shadow) shadow_intensity += 1.0f;
            }
            shadow_intensity /= SHADOW_SAMPLES;
        } else {
            bool in_shadow = intersect(abs_dir, hit + bias, objects, closest_object, nullptr, t);
            if (!in_shadow) shadow_intensity = 1.0f;
        }

#ifdef ATTENUATION
        float distance = light->distance(hit);
        float attenuation = light->intensity / std::pow(distance, ATTENUATION_EXPONENT);
        attenuation = std::min(1.0f, attenuation);
#else
        float attenuation = light->intensity;
#endif
        direct_color += closest_object->color * light->color * attenuation * std::max(0.0f, normal.dot(abs_dir)) / M_PI * shadow_intensity;
    }

    // indirect lighting
    vec3 indirect_color = {0, 0, 0};
    for (int i=0; i<INDIRECT_SAMPLES; i++) {
        vec3 indirect_dir = hemisphere_sample(normal);
    }

    return direct_color; */
}
void render(float fov_rad, const vec3& look_from, const vec3& look_at, const vec3& up, float image_distance, const vec3& bg_color, const std::vector<std::shared_ptr<object_t>>& objects, const std::vector<std::shared_ptr<light_t>>& lights, image_t& image) {
    std::pair<int, int> res = {image.height, image.width};

#define height first
#define width second
    std::pair<float, float> v_res = {
        2.0 * image_distance * std::tan(fov_rad / 2) * static_cast<float>(res.height) / res.width,
        2.0 * image_distance * std::tan(fov_rad / 2)};
    float cell_width = v_res.width / res.width;

    mat4 camera = mat4_constructors::camera(look_from, look_at, up);
    for (int h = 0; h < res.height; h++) {
        for (int w = 0; w < res.width; w++) {
            vec3 ray_d = {
                w * cell_width - v_res.width / 2 + cell_width / 2,
                h * cell_width - v_res.height / 2 + cell_width / 2,
                -image_distance};
            ray_d = camera.transform_dir(ray_d).normalize();

            // abs
            vec3 color = raycast(ray_d, look_from, objects, lights, bg_color, RENDER_DEPTH);
            // std::cout << color << std::endl;
            image.set_pixel(w, h, color);
            // break;
        }
    }
#undef height
#undef width
}