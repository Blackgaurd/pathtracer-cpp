#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>

#include "sfml_linalg.h"

#define DEG2RAD M_PI / 180

using mat4 = sf::Glsl::Mat4;
using vec3 = sf::Glsl::Vec3;
using vec4 = sf::Glsl::Vec4;
using vec2 = sf::Glsl::Vec2;
using ivec2 = sf::Glsl::Ivec2;

std::ostream& operator<<(std::ostream& os, const vec3& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::array<float, 16>& mat) {
    // print like this
    // 1 2 3 4
    // 5 6 7 8

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            os << mat[i * 4 + j] << ' ';
        }
        os << '\n';
    }
    return os;
}

struct Camera {
    // i do not understand quaternions
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
    float fov, distance;
    vec3 pos;
    vec3 forward, up, right;  // centered about origin
    vec3 const_up;            // used for rotating
    std::array<float, 16> transform;

#define set_row(row, vec)           \
    transform[row * 4] = vec.x;     \
    transform[row * 4 + 1] = vec.y; \
    transform[row * 4 + 2] = vec.z;

    Camera() = default;
    Camera(const vec3& pos, const vec3& forward, const vec3& up, const ivec2& res, float fov,
           float distance) {
        this->pos = pos;
        this->forward = normalize(forward);
        this->right = normalize(cross(forward, up));
        this->up = normalize(cross(right, forward));
        this->const_up = this->up;

        if (std::abs(dot(forward, up)) > 0.999) {
            std::cerr << "Up vector is too close to forward vector" << std::endl;
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
    }

    // rotation and movement inspire
    // by games like minecraft
    void rotate(Direction dir, float angle) {
        switch (dir) {
            case LEFT: {
                forward = normalize(forward * std::cos(angle) - right * std::sin(angle));
                right = normalize(cross(forward, const_up));
                up = normalize(cross(right, forward));
                break;
            }
            case RIGHT: {
                forward = normalize(forward * std::cos(angle) + right * std::sin(angle));
                right = normalize(cross(forward, const_up));
                up = normalize(cross(right, forward));
                break;
            }
            case UP: {
                forward = normalize(forward * std::cos(angle) + up * std::sin(angle));
                up = normalize(cross(right, forward));
                break;
            }
            case DOWN: {
                forward = normalize(forward * std::cos(angle) - up * std::sin(angle));
                up = normalize(cross(right, forward));
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
        // according to const_up
        switch (dir) {
            case UP: {
                pos += const_up * amount;
                break;
            }
            case DOWN: {
                pos -= const_up * amount;
                break;
            }
            case FORWARD: {
                vec3 const_forward = normalize(cross(const_up, right));
                pos += const_forward * amount;
                break;
            }
            case BACKWARD: {
                vec3 const_forward = normalize(cross(const_up, right));
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

struct Material {
    enum Type {
        // hard coded to match the shader
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

    vec3 pos = {0, 0, 0}, forward = {0, 0, -1}, b_up = {0, 1, 0};
    Camera camera(pos, forward, b_up, resolution, 60 * DEG2RAD, 1);

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
        sf::Event event;
        bool camera_changed = false;
        while (window.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Closed: {
                    window.close();
                    break;
                }
                case sf::Event::KeyPressed: {
                    switch (event.key.code) {
                        case sf::Keyboard::Escape: {
                            window.close();
                            break;
                        }
                        // rotate with arrow keys
                        case sf::Keyboard::Left: {
                            camera.rotate(Camera::LEFT, 1 * DEG2RAD);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Right: {
                            camera.rotate(Camera::RIGHT, 1 * DEG2RAD);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Up: {
                            camera.rotate(Camera::UP, 1 * DEG2RAD);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Down: {
                            camera.rotate(Camera::DOWN, 1 * DEG2RAD);
                            camera_changed = true;
                            break;
                        }
                        // move with wasd space z
                        case sf::Keyboard::W: {
                            camera.move(Camera::FORWARD, 0.1);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::S: {
                            camera.move(Camera::BACKWARD, 0.1);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::A: {
                            camera.move(Camera::LEFT, 0.1);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::D: {
                            camera.move(Camera::RIGHT, 0.1);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Space: {
                            camera.move(Camera::UP, 0.1);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Z: {
                            camera.move(Camera::DOWN, 0.1);
                            camera_changed = true;
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        pathtracer.setUniform("frame", frame++);
        pathtracer.setUniform("frame_count", frame_count++);
        pathtracer.setUniform("prev_frame", texture.getTexture());

        pathtracer.setUniform("look_from", camera.pos);
        pathtracer.setUniform("v_res", camera.v_res);
        pathtracer.setUniform("image_distance", camera.distance);
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