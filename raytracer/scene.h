#pragma once

#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include "camera.h"
#include "image.h"
#include "light.h"
#include "object.h"
#include "rng.h"

#define FLOAT_MAX std::numeric_limits<float>::max()

vec3 hemisphere_sample(const vec3& normal) {
    float u = rng.rand01(), v = rng.rand01();
    float theta = std::acos(2 * u - 1) - M_PI_2;
    float phi = 2 * M_PI * v;
    vec3 sample = {std::cos(theta) * std::cos(phi),
                   std::cos(theta) * std::sin(phi), std::sin(theta)};
    return sample.dot(normal) < 0 ? -sample : sample;
}

struct Scene {
    using ObjectPtr = std::shared_ptr<Object>;
    using LightPtr = std::shared_ptr<Light>;

    std::vector<ObjectPtr> objects;
    std::vector<LightPtr> lights;
    vec3 bg_color = {0, 0, 0};

    // settings
    float shift_bias = 1e-4;
    int shadow_samples = 64;
    int indirect_samples = 64;
    // int anti_aliasing = 1;

    Scene() = default;

    template <typename T, typename... Args>
    void add_object(Args&&... args) {
        objects.push_back(std::make_shared<T>(std::forward<Args>(args)...));
    }
    void add_object(const ObjectPtr& obj) {
        objects.push_back(obj);
    }
    template <typename T, typename... Args>
    void add_light(Args&&... args) {
        lights.push_back(std::make_shared<T>(std::forward<Args>(args)...));
    }
    void add_light(const LightPtr& light) {
        lights.push_back(light);
    }

    void render(Image& image, const Camera& camera, int depth) {
        if (camera.res != image.res)
            throw std::runtime_error("Camera and image resolution mismatch");

        Resolution res = image.res;
        int pixels = res.width * res.height;
        std::cout << std::fixed << std::setprecision(2);
        for (int h = 0; h < res.height; h++) {
            for (int w = 0; w < res.width; w++) {
                std::cout << "Rendering: "
                          << 100.0 * (h * res.width + w) / pixels << "%"
                          << std::flush << "\r";
                vec3 ray_o, ray_d;
                camera.get_ray(w, h, ray_o, ray_d);
                vec3 color = raycast(ray_o, ray_d, depth);
                image.set_pixel(w, h, color);
            }
        }
    }
    float light_intensity(const vec3& ray_o, const vec3& ray_d,
                          const LightPtr& light, const ObjectPtr& ignore) {
        // returns:
        // 0 if no light gets to the point
        // 1 if all light gets to the point
        // 0 < x < 1 if some light gets to the point
        if (light->soft_shadows) {
            float factor = 0;
            for (int i = 0; i < shadow_samples; i++) {
                float t;
                vec3 rand_dir = light->random_dir(ray_o);
                ObjectPtr obj = intersect(ray_o, rand_dir, ignore, t);
                if (!obj) factor += light->intensity;
            }
            return factor / shadow_samples;
        } else {
            float t;
            vec3 abs_dir = light->absolute_dir(ray_o);
            ObjectPtr obj = intersect(ray_o, abs_dir, ignore, t);
            return obj ? 0 : light->intensity;
        }
    }
    vec3 direct_lighting(const vec3& hit_p, const vec3& normal,
                         const LightPtr& light) {
        vec3 abs_dir = light->absolute_dir(hit_p);
        return light->color * std::max(0.0f, normal.dot(abs_dir)) / M_PI;
    }
    vec3 indirect_lighting(const vec3& ray_o, const vec3& normal,
                           int cur_depth) {
        vec3 color = {0, 0, 0};
        for (int i = 0; i < indirect_samples; i++) {
            vec3 sample_dir = hemisphere_sample(normal);
            color += raycast(ray_o, sample_dir, cur_depth - 1);
        }
        return color / indirect_samples;
    }
    ObjectPtr intersect(const vec3& ray_o, const vec3& ray_d,
                        const ObjectPtr& ignore, float& hit_t) {
        // find first object hit
        hit_t = FLOAT_MAX;
        ObjectPtr hit_obj = nullptr;

        float t;
        for (const ObjectPtr& obj : objects) {
            if (obj == ignore) continue;
            if (obj->intersect(ray_o, ray_d, t) && t < hit_t) {
                hit_t = t;
                hit_obj = obj;
            }
        }
        return hit_obj;
    }
    vec3 raycast(const vec3& ray_o, const vec3& ray_d, int depth) {
        if (depth == 0) return {0, 0, 0};

        float hit_t;
        ObjectPtr hit_obj = intersect(ray_o, ray_d, nullptr, hit_t);
        if (!hit_obj) return bg_color;

        vec3 hit_p = ray_o + ray_d * hit_t;
        vec3 normal = hit_obj->normal(hit_p);
        vec3 bias = normal * shift_bias;

        vec3 direct_color_sum = {0, 0, 0};
        for (const LightPtr& light : lights) {
            float intensity =
                light_intensity(hit_p + bias, ray_d, light, hit_obj);
            vec3 direct_color = direct_lighting(hit_p + bias, normal, light);
            direct_color_sum += direct_color * intensity;
        }
        vec3 indirect_color = indirect_lighting(hit_p + bias, normal, depth);
        vec3 final_color = hit_obj->color * (direct_color_sum + indirect_color);

        // normalize the color
        float max_chan = final_color.max();
        if (max_chan > 1) final_color /= max_chan;

        return final_color;
    }
};