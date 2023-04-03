#pragma once

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
#define SHADOW_SAMPLES 64

vec3 raycast(const vec3& ray_d, const vec3& ray_o, const std::vector<std::shared_ptr<object_t>>& objects, const std::vector<std::shared_ptr<light_t>>& lights, const vec3& bg_color) {
    // check intersections
    float t = std::numeric_limits<float>::max() / 2;
    std::shared_ptr<object_t> closest_object = nullptr;
    for (auto& object : objects) {
        float t_candidate;
        if (object->intersect(ray_o, ray_d, t_candidate) && t_candidate < t) {
            t = t_candidate;
            closest_object = object;
        }
    }
    if (closest_object == nullptr) return bg_color;

    // do lighting
    vec3 hit = ray_o + ray_d * t;
    vec3 normal = closest_object->normal(hit);
    vec3 bias = normal * BIAS;

    vec3 color = {0, 0, 0};
    for (auto& light : lights) {
        vec3 light_dir = light->absolute_dir(hit);
        // shadows
        float shadow_intensity = 0;
        if (light->soft_shadows) {
            for (int i = 0; i < SHADOW_SAMPLES; i++) {
                light_dir = light->random_dir(hit);
                bool in_shadow = false;
                for (auto& object : objects) {
                    if (object == closest_object) continue;
                    float t_candidate;
                    if (object->intersect(hit + bias, light_dir, t_candidate)) {
                        in_shadow = true;
                        break;
                    }
                }
                if (in_shadow) continue;
                shadow_intensity += 1.0f;
            }
            shadow_intensity /= SHADOW_SAMPLES;
        } else {
            bool in_shadow = false;
            for (auto& object : objects) {
                if (object == closest_object) continue;
                float t_candidate;
                if (object->intersect(hit + bias, light_dir, t_candidate)) {
                    in_shadow = true;
                    break;
                }
            }
            if (in_shadow) continue;
            shadow_intensity = 1.0f;
        }

        color += closest_object->color * light->color * light->intensity * std::max(0.0f, normal.dot(light_dir)) / M_PI * shadow_intensity;
    }
    return color;
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
            vec3 color = raycast(ray_d, look_from, objects, lights, bg_color);
            // std::cout << color << std::endl;
            image.set_pixel(w, h, color);
            // break;
        }
    }
#undef height
#undef width
}