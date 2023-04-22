#pragma once

#include "linalg.h"
#include "rng.h"

struct fResolution {
    float width, height;

    fResolution() = default;
    fResolution(float width, float height) : width(width), height(height) {}
};

// isometric projection camera
struct Camera {
    Resolution res;     // in pixels
    fResolution v_res;  // in world coordinates
    float cell_size;    // in world coordinates
    float image_distance;
    vec3 look_from, look_at, up;
    mat4 transform;

    Camera() = default;
    Camera(const Resolution& resolution, float fov, float image_distance, const vec3& look_from,
           const vec3& look_at, const vec3& up)
        : res(resolution),
          image_distance(image_distance),
          look_from(look_from),
          look_at(look_at),
          up(up) {
        vec3 forward = (look_from - look_at).normalize();

        if (forward.angle(up) < EPS || forward.angle(-up) < EPS) {
            std::cerr << "forward and up vectors are parallel" << std::endl;
            exit(1);
            return;
        }

        vec3 left = up.cross(forward).normalize();
        vec3 true_up = forward.cross(left).normalize();

        transform = mat4(0);
        for (size_t i = 0; i < 4; i++) transform[i][i] = 1;

#define set_row(i, v)      \
    transform[i][0] = v.x; \
    transform[i][1] = v.y; \
    transform[i][2] = v.z;

        set_row(0, left);
        set_row(1, true_up);
        set_row(2, forward);
        set_row(3, look_from);
#undef set_row

        v_res = {2.0f * image_distance * std::tan(fov / 2),
                 2.0f * image_distance * std::tan(fov / 2) * static_cast<float>(resolution.height) /
                     resolution.width};
        cell_size = v_res.width / resolution.width;
    }

    void get_ray(int w, int h, vec3& ray_o, vec3& ray_d) const {
        float jitter_w = rng.rand01() * cell_size, jitter_h = rng.rand01() * cell_size;
        ray_d = {w * cell_size - v_res.width / 2 + jitter_w,
                 h * cell_size - v_res.height / 2 + jitter_h, -image_distance};
        ray_d = transform.transform_dir(ray_d).normalize();
        ray_o = look_from;
    }
};

// orthographic projection camera
struct OrthoCamera : public Camera {
    // todo
};