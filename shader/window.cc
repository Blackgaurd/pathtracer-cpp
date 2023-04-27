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
struct Triangle {
    vec3 v1, v2, v3;
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

    vec3 pos = {278, 278, -500}, forward = {0, 0, 1}, b_up = {0, 1, 0};
    Camera camera(pos, forward, b_up, resolution, 60 * DEG2RAD, 1);

    Material red_diffuse = {Material::DIFF, {1, 0, 0}, {0, 0, 0}, 0};
    Material green_diffuse = {Material::DIFF, {0, 1, 0}, {0, 0, 0}, 0};
    Material white_diffuse = {Material::DIFF, {1, 1, 1}, {0, 0, 0}, 0};
    Material white_emit = {Material::EMIT, {0, 0, 0}, {1, 1, 1}, 0};

    std::vector<Triangle> triangles;
    vec3 f1 = vec3(552.8, 0, 0), f2 = vec3(0, 0, 0), f3 = vec3(0, 0, 559.2),
         f4 = vec3(549.6, 0, 559.2);
    triangles.push_back(Triangle{f1, f2, f3, white_diffuse});
    triangles.push_back(Triangle{f4, f3, f1, white_diffuse});
    vec3 l1 = vec3(343, 548.7, 227), l2 = vec3(343, 548.7, 332), l3 = vec3(213, 548.7, 332),
         l4 = vec3(213, 548.7, 227);
    triangles.push_back(Triangle{l1, l2, l3, white_emit});
    triangles.push_back(Triangle{l4, l3, l1, white_emit});
    vec3 c1 = vec3(556, 548.8, 0), c2 = vec3(0, 548.8, 0), c3 = vec3(0, 548.8, 559.2),
         c4 = vec3(556.0, 548.8, 559.2);
    triangles.push_back(Triangle{c1, c2, c3, white_diffuse});
    triangles.push_back(Triangle{c4, c3, c1, white_diffuse});
    vec3 b1 = vec3(549.6, 0, 559.2), b2 = vec3(0, 0, 559.2), b3 = vec3(0, 548.8, 559.2),
         b4 = vec3(556, 548.8, 559.2);
    triangles.push_back(Triangle{b1, b2, b3, white_diffuse});
    triangles.push_back(Triangle{b4, b3, b1, white_diffuse});
    vec3 r1 = vec3(0, 0, 559.2), r2 = vec3(0, 0, 0), r3 = vec3(0, 548.8, 0),
         r4 = vec3(0, 548.8, 559.2);
    triangles.push_back(Triangle{r1, r2, r3, green_diffuse});
    triangles.push_back(Triangle{r4, r3, r1, green_diffuse});
    vec3 lw1 = vec3(552.8, 0, 0), lw2 = vec3(549.6, 0, 559.2), lw3 = vec3(556, 548.8, 559.2),
         lw4 = vec3(556, 548.8, 0);
    triangles.push_back(Triangle{lw1, lw2, lw3, red_diffuse});
    triangles.push_back(Triangle{lw4, lw3, lw1, red_diffuse});
    vec3 sb1 = vec3(130, 165, 65), sb2 = vec3(82, 165, 225), sb3 = vec3(240, 165, 272),
         sb4 = vec3(290, 165, 114);
    triangles.push_back(Triangle{sb1, sb2, sb3, white_diffuse});
    triangles.push_back(Triangle{sb4, sb3, sb1, white_diffuse});
    vec3 sb5 = vec3(290, 0, 114), sb6 = vec3(290, 165, 114), sb7 = vec3(240, 165, 272),
         sb8 = vec3(240, 0, 272);
    triangles.push_back(Triangle{sb5, sb6, sb7, white_diffuse});
    triangles.push_back(Triangle{sb8, sb7, sb5, white_diffuse});
    vec3 sb9 = vec3(130, 0, 65), sb10 = vec3(130, 165, 65), sb11 = vec3(290, 165, 114),
         sb12 = vec3(290, 0, 114);
    triangles.push_back(Triangle{sb9, sb10, sb11, white_diffuse});
    triangles.push_back(Triangle{sb12, sb11, sb9, white_diffuse});
    vec3 sb13 = vec3(82, 0, 225), sb14 = vec3(82, 165, 225), sb15 = vec3(130, 165, 65),
         sb16 = vec3(130, 0, 65);
    triangles.push_back(Triangle{sb13, sb14, sb15, white_diffuse});
    triangles.push_back(Triangle{sb16, sb15, sb13, white_diffuse});
    vec3 sb17 = vec3(240, 0, 272), sb18 = vec3(240, 165, 272), sb19 = vec3(82, 165, 225),
         sb20 = vec3(82, 0, 225);
    triangles.push_back(Triangle{sb17, sb18, sb19, white_diffuse});
    triangles.push_back(Triangle{sb20, sb19, sb17, white_diffuse});

    int triangle_count = triangles.size();
    pathtracer.setUniform("triangle_count", triangle_count);
    for (int i = 0; i < triangle_count; i++) {
        std::string name = "triangles[" + std::to_string(i) + "].";
        pathtracer.setUniform(name + "v1", triangles[i].v1);
        pathtracer.setUniform(name + "v2", triangles[i].v2);
        pathtracer.setUniform(name + "v3", triangles[i].v3);
        name += "material.";
        pathtracer.setUniform(name + "type", triangles[i].material.type);
        pathtracer.setUniform(name + "color", triangles[i].material.color);
        pathtracer.setUniform(name + "emit_color", triangles[i].material.emit_color);
        pathtracer.setUniform(name + "roughness", triangles[i].material.roughness);
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
    bool camera_changed = true;
    while (window.isOpen()) {
        sf::Event event;
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

        if (camera_changed) {
            pathtracer.setUniform("look_from", camera.pos);
            pathtracer.setUniform("v_res", camera.v_res);
            pathtracer.setUniform("image_distance", camera.distance);
            pathtracer.setUniform("camera", mat4(camera.transform.data()));
            texture.clear();
            frame_count = 0;
            camera_changed = false;
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