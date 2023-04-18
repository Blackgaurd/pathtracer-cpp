#pragma once

#include <memory>
#include <vector>

#include "camera.h"
#include "image.h"
#include "linalg.h"
#include "object.h"

#define FLOAT_INF 1e20

struct Scene {
    using ObjectPtr = std::shared_ptr<Object>;

    std::vector<ObjectPtr> objects;

    float shift_bias = 1e-4;

    Scene() = default;

    template <typename T, typename... Args>
    void add_object(Args&&... args) {
        objects.push_back(std::make_shared<T>(std::forward<Args>(args)...));
    }

    void render(const Camera& camera, Image& image, int depth = 5,
                int samples = 1) {
        Resolution res = image.res;
        for (int h = 0; h < res.height; h++) {
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
    ObjectPtr intersect(const vec3& ray_o, const vec3& ray_d, float& hit_t,
                        const ObjectPtr& ignore = nullptr) {
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
        ObjectPtr hit_obj = intersect(ray_o, ray_d, hit_t);
        if (!hit_obj) return {0, 0, 0};

        if (hit_obj->material->type == EMIT) {
            return hit_obj->material->emission;
        }

        vec3 hit_p = ray_o + ray_d * hit_t;
        vec3 hit_n = hit_obj->normal(hit_p);
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