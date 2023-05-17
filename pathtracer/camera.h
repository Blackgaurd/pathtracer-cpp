#pragma once

#include <iostream>

#include "linalg.h"
#include "rng.h"

// isometric projection camera
struct Camera {
    enum Direction {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN,
    };

    ivec2 res;
    vec2 v_res;
    float fov, distance, cell_size;
    vec3 pos;
    vec3 forward, up, right;  // centered about origin
    vec3 world_up;            // used for rotating
    std::array<float, 16> transform;

#define set_row(row, vec)           \
    transform[row * 4] = vec.x;     \
    transform[row * 4 + 1] = vec.y; \
    transform[row * 4 + 2] = vec.z;

    Camera() = default;
    Camera(const vec3& pos, const vec3& forward, const vec3& up, const ivec2& res, float fov,
           float distance) {
        this->pos = pos;
        this->forward = forward.normalize();
        this->right = forward.cross(up).normalize();
        this->up = right.cross(forward).normalize();
        this->world_up = this->up;

        if (std::abs(this->forward.dot(this->up)) > 0.999) {
            std::cerr << "Up vector is too close to forward vector" << '\n';
            std::cerr << "forward: " << this->forward << ", up: " << this->up << '\n';
            return;
        }

        this->res = res;
        this->fov = fov;
        this->distance = distance;
        v_res = {2 * distance * std::tan(fov / 2),
                 2 * distance * std::tan(fov / 2) * res.y / res.x};

        for (int i = 0; i < 4; i++) transform[i * 4 + i] = 1;

        set_row(0, this->right);
        set_row(1, this->up);
        set_row(2, -this->forward);
        set_row(3, this->pos);

        this->cell_size = v_res.x / res.x;
    }

    void get_ray(int w, int h, vec3& ray_o, vec3& ray_d) const {
        ray_d = vec3((w + rng.rand01()) * cell_size - v_res.x / 2,
                     (h + rng.rand01()) * cell_size - v_res.y / 2, -distance);
        // transform[x * 4 + y] is the same as transform[x][y] if mat4
        ray_d =
            vec3(ray_d.dot(vec3(transform[0 * 4 + 0], transform[1 * 4 + 0], transform[2 * 4 + 0])),
                 ray_d.dot(vec3(transform[0 * 4 + 1], transform[1 * 4 + 1], transform[2 * 4 + 1])),
                 ray_d.dot(vec3(transform[0 * 4 + 2], transform[1 * 4 + 2], transform[2 * 4 + 2])))
                .normalize();
        ray_o = pos;
    }

    // rotation and movement inspire
    // by games like minecraft
    void rotate(Direction dir, float angle) {
        std::cout << "rotate " << dir << " " << angle << '\n';
        switch (dir) {
            case LEFT: {
                forward = (forward * std::cos(angle) - right * std::sin(angle)).normalize();
                right = forward.cross(world_up).normalize();
                up = right.cross(forward).normalize();
                break;
            }
            case RIGHT: {
                forward = (forward * std::cos(angle) + right * std::sin(angle)).normalize();
                right = forward.cross(world_up).normalize();
                up = right.cross(forward).normalize();
                break;
            }
            case UP: {
                forward = (forward * std::cos(angle) + up * std::sin(angle)).normalize();
                up = right.cross(forward).normalize();
                break;
            }
            case DOWN: {
                forward = (forward * std::cos(angle) - up * std::sin(angle)).normalize();
                up = right.cross(forward).normalize();
                break;
            }
            default:
                break;
        }
        set_row(0, right);
        set_row(1, up);
        set_row(2, -forward);
    }
    void move(Direction dir, float amount) {
        // according to world_up
        switch (dir) {
            case UP: {
                pos += world_up * amount;
                break;
            }
            case DOWN: {
                pos -= world_up * amount;
                break;
            }
            case FORWARD: {
                vec3 const_forward = world_up.cross(right).normalize();
                pos += const_forward * amount;
                break;
            }
            case BACKWARD: {
                vec3 const_forward = world_up.cross(right).normalize();
                pos -= const_forward * amount;
                break;
            }
            case LEFT: {
                pos -= right * amount;
                break;
            }
            case RIGHT: {
                pos += right * amount;
                break;
            }
            default:
                break;
        }
        set_row(3, pos);
    }
#undef set_row
};