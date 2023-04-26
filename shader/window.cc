#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>

#include "sfml_linalg.h"

using mat4 = sf::Glsl::Mat4;
using vec3 = sf::Glsl::Vec3;
using vec4 = sf::Glsl::Vec4;
using vec2 = sf::Glsl::Vec2;
using ivec2 = sf::Glsl::Ivec2;

struct Camera {
    ivec2 res;
    vec2 v_res;
    float fov, image_distance;
    vec3 look_from, look_at, up;
    std::array<float, 16> transform;

    Camera() = default;
    Camera(const ivec2& res, float fov, float image_distance, const vec3& look_from,
           const vec3& look_at, const vec3& up)
        : res(res),
          fov(fov),
          image_distance(image_distance),
          look_from(look_from),
          look_at(look_at),
          up(up) {
        vec3 forward = normalize(look_from - look_at);
        if (std::abs(dot(forward, up)) > 0.999) {
            std::cerr << "Up vector is too close to forward vector" << std::endl;
            return;
        }
        vec3 left = normalize(cross(up, forward));
        vec3 true_up = normalize(cross(forward, left));

        for (int i = 0; i < 4; i++) transform[i * 4 + i] = 1;

#define set_row(row, vec)           \
    transform[row * 4] = vec.x;     \
    transform[row * 4 + 1] = vec.y; \
    transform[row * 4 + 2] = vec.z;

        set_row(0, left);
        set_row(1, true_up);
        set_row(2, forward);
        set_row(3, look_from);
#undef set_row

        float fov_rad = fov * M_PI / 180;
        v_res = {2 * image_distance * std::tan(fov_rad / 2),
                 2 * image_distance * std::tan(fov_rad / 2) * res.y / res.x};
    }

    void update_view(const vec3& look_from, const vec3& look_at, const vec3& up) {
        this->look_from = look_from;
        this->look_at = look_at;
        this->up = up;
        vec3 forward = normalize(look_from - look_at);
        if (std::abs(dot(forward, up)) > 0.999) {
            std::cerr << "Up vector is too close to forward vector" << std::endl;
            return;
        }
        vec3 left = normalize(cross(up, forward));
        vec3 true_up = normalize(cross(forward, left));

        for (int i = 0; i < 4; i++) transform[i * 4 + i] = 1;

#define set_row(row, vec)           \
    transform[row * 4] = vec.x;     \
    transform[row * 4 + 1] = vec.y; \
    transform[row * 4 + 2] = vec.z;

        set_row(0, left);
        set_row(1, true_up);
        set_row(2, forward);
        set_row(3, look_from);
#undef set_row
    }
};

struct Material {
    enum Type {
        EMIT = 1,
        DIFF = 2,
        SPEC = 3,
    } type;
    vec3 color;
    vec3 emit_color;
    float roughness;
};
struct Sphere {
    vec3 center;
    float radius;
    Material material;
};

bool load_shader(const std::string& filename, sf::Shader& shader) {
    if (!sf::Shader::isAvailable()) {
        // shaders are not available...
        std::cerr << "Shaders are not available" << std::endl;
        return false;
    }
    if (!shader.loadFromFile(filename, sf::Shader::Fragment)) {
        std::cerr << "Failed to load shader" << std::endl;
        return false;
    }
    return true;
}

int main() {
    const ivec2 resolution(800, 800);
    sf::RenderWindow window(sf::VideoMode(resolution.x, resolution.y), "My window");

    sf::Shader pathtracer;
    if (!load_shader("shader/pathtracer.frag", pathtracer)) return 1;

    vec3 look_from = {0, 0, 0}, look_at = {0, 0, -1}, up = {0, 1, 0};
    Camera camera(resolution, 60, 1, look_from, look_at, up);
    const int sphere_count = 6;
    const Sphere spheres[sphere_count] = {
        Sphere{{0, 0, -3}, 0.5, {Material::DIFF, {0.8, 0.8, 0.8}, {0, 0, 0}, 0}},
        Sphere{{0, 0, -3}, 0.1, {Material::EMIT, {0, 0, 0}, {1, 1, 1}, 0}},
        Sphere{{1, 1, -2}, 0.5, {Material::EMIT, {0, 0, 0}, {1, 1, 0}, 0}},
        Sphere{{-1, -1, -2}, 0.5, {Material::EMIT, {0, 0, 0}, {0, 1, 1}, 0}},
        Sphere{{1, -1, -2}, 0.5, {Material::EMIT, {0, 0, 0}, {0, 1, 0}, 0}},
        Sphere{{-1, 1, -2}, 0.5, {Material::EMIT, {0, 0, 0}, {1, 0, 0}, 0}}};
    pathtracer.setUniform("sphere_count", sphere_count);
    for (int i = 0; i < sphere_count; i++) {
        std::string name = "spheres[" + std::to_string(i) + "].";
        pathtracer.setUniform(name + "center", spheres[i].center);
        pathtracer.setUniform(name + "radius", spheres[i].radius);

        name += "material.";
        pathtracer.setUniform(name + "type", spheres[i].material.type);
        pathtracer.setUniform(name + "color", spheres[i].material.color);
        pathtracer.setUniform(name + "emit_color", spheres[i].material.emit_color);
        pathtracer.setUniform(name + "roughness", spheres[i].material.roughness);
    }

    const int FPS = 24;
    window.setFramerateLimit(FPS);
    const sf::RectangleShape screen(sf::Vector2f(resolution.x, resolution.y));
    pathtracer.setUniform("resolution", camera.res);

    int frame = 1;
    int frame_count = 0;
    sf::RenderTexture texture;
    texture.create(resolution.x, resolution.y);

    std::cout << std::fixed << std::setprecision(2);
    std::chrono::high_resolution_clock::time_point prev_time =
        std::chrono::high_resolution_clock::now();
    while (window.isOpen()) {
        bool camera_changed = false;
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed: {
                    window.close();
                    break;
                }
                case sf::Event::KeyPressed: {
                    bool update_camera = false;
                    switch (event.key.code) {
                        case sf::Keyboard::Escape: {
                            window.close();
                            break;
                        }

                        // pan camera
                        case sf::Keyboard::Up: {
                            look_at.y += 0.01;
                            update_camera = true;
                            break;
                        }
                        case sf::Keyboard::Down: {
                            look_at.y -= 0.01;
                            update_camera = true;
                            break;
                        }
                        case sf::Keyboard::Right: {
                            look_at.x += 0.01;
                            update_camera = true;
                            break;
                        }
                        case sf::Keyboard::Left: {
                            look_at.x -= 0.01;
                            update_camera = true;
                            break;
                        }

                        // move camera
                        case sf::Keyboard::W: {
                            look_from.z -= 0.01;
                            look_at.z -= 0.01;
                            update_camera = true;
                            break;
                        }
                        case sf::Keyboard::S: {
                            look_from.z += 0.01;
                            look_at.z += 0.01;
                            update_camera = true;
                            break;
                        }
                        case sf::Keyboard::A: {
                            look_from.x -= 0.01;
                            look_at.x -= 0.01;
                            update_camera = true;
                            break;
                        }
                        case sf::Keyboard::D: {
                            look_from.x += 0.01;
                            look_at.x += 0.01;
                            update_camera = true;
                            break;
                        }
                        default:
                            break;
                    }
                    if (!update_camera) break;
                    camera.update_view(look_from, look_at, up);
                    camera_changed = true;
                    break;
                }
                default:
                    break;
            }
        }
        pathtracer.setUniform("frame", frame++);
        pathtracer.setUniform("frame_count", frame_count++);
        pathtracer.setUniform("prev_frame", texture.getTexture());

        pathtracer.setUniform("look_from", look_from);
        pathtracer.setUniform("v_res", camera.v_res);
        pathtracer.setUniform("image_distance", camera.image_distance);
        pathtracer.setUniform("camera", mat4(camera.transform.data()));

        if (camera_changed) {
            texture.clear();
            frame_count = 0;
        }
        texture.draw(screen, &pathtracer);
        texture.display();

        window.clear();
        window.draw(sf::Sprite(texture.getTexture()));
        window.display();

        float fps = 1.0 / std::chrono::duration_cast<std::chrono::duration<double>>(
                              std::chrono::high_resolution_clock::now() - prev_time)
                              .count();
        prev_time = std::chrono::high_resolution_clock::now();
        std::cout << "\rFPS: " << fps << std::flush;
    }

    texture.getTexture().copyToImage().saveToFile("output.png");
    std::cout << std::endl;
    std::cout << "Saved output.png" << std::endl;
}