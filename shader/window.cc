#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <iomanip>
#include <iostream>

#include "sfml_linalg.h"

#define DEG2RAD M_PI / 180
#define FLOAT_INF 1e30

using mat4 = sf::Glsl::Mat4;
using vec3 = sf::Glsl::Vec3;
using vec4 = sf::Glsl::Vec4;
using vec2 = sf::Glsl::Vec2;
using ivec2 = sf::Glsl::Ivec2;

#define vec3_from(x) vec3(x, x, x)

std::ostream& operator<<(std::ostream& os, const vec3& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::array<float, 16>& mat) {
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
            std::cerr << "Up vector is too close to forward vector" << '\n';
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

vec3 component_min(const vec3& a, const vec3& b) {
    return vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
}
struct AABB {
    vec3 lb = vec3_from(FLOAT_INF), rt = vec3_from(-FLOAT_INF);

    AABB() = default;
    AABB(const vec3& lb, const vec3& rt) : lb(lb), rt(rt) {}

    void merge(const AABB& other) {
        lb = component_min(lb, other.lb);
        rt = component_min(rt, other.rt);
    }
    void merge(const vec3& point) {
        lb = component_min(lb, point);
        rt = component_min(rt, point);
    }
    float area() const {
        // half of surface area
        vec3 diff = rt - lb;
        return diff.x * diff.y + diff.x * diff.z + diff.y * diff.z;
    }
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

    Material() = default;
    Material(Type type, const vec3& color, const vec3& emit_color, float roughness)
        : type(type), color(color), emit_color(emit_color), roughness(roughness) {}
};
struct Triangle {
    AABB aabb;
    vec3 centroid;
    vec3 v1, v2, v3;
    Material material;

    Triangle() = default;
    Triangle(const vec3& v1, const vec3& v2, const vec3& v3, const Material& material)
        : v1(v1), v2(v2), v3(v3), material(material) {
        aabb.merge(v1);
        aabb.merge(v2);
        aabb.merge(v3);
        centroid = (v1 + v2 + v3) / 3;
    }
};

struct BVHNode {
    AABB aabb;
    int left, right;
    int tri_start, tri_end;

    BVHNode() = default;
    BVHNode(int left, int right, int tri_start, int tri_end)
        : left(left), right(right), tri_start(tri_start), tri_end(tri_end) {}
};
struct BVH {
    std::vector<Triangle> triangles;
    std::vector<int> tri_indices;
    std::vector<BVHNode> nodes;

    BVH() = default;
    BVH(const std::vector<Triangle>& triangles) {
        this->triangles = triangles;
        build();
    }

    void add_tri(const Triangle& tri) {
        triangles.push_back(tri);
    }
    void build() {
        // queue based bvh building
        tri_indices.resize(triangles.size());
        for (int i = 0; i < int(tri_indices.size()); i++) tri_indices[i] = i;

        nodes.reserve(triangles.size() * 2);
        nodes.push_back(BVHNode(-1, -1, 0, triangles.size() - 1));

        std::deque<int> q;
        q.push_back(0);
        while (!q.empty()) {
            int cur = q.front();
            q.pop_front();

            int best_axis = -1;
            float split_pos = 0, min_cost = FLOAT_INF;
            for (int axis = 0; axis < 3; axis++) {
                for (int i = nodes[cur].tri_start; i <= nodes[cur].tri_end; i++) {
                    const Triangle& tri = triangles[tri_indices[i]];
                    float pos = get(tri.centroid, axis);
                    int left_cnt = 0, right_cnt = 0;
                    AABB left_aabb, right_aabb;
                    for (int j = nodes[cur].tri_start; j <= nodes[cur].tri_end; j++) {
                        const Triangle& other = triangles[tri_indices[j]];
                        if (get(other.centroid, axis) < pos) {
                            left_cnt++;
                            left_aabb.merge(other.aabb);
                        } else {
                            right_cnt++;
                            right_aabb.merge(other.aabb);
                        }
                    }
                    if (left_cnt == 0 || right_cnt == 0) continue;

                    float cost = left_cnt * left_aabb.area() + right_cnt * right_aabb.area();
                    if (cost < min_cost) {
                        min_cost = cost;
                        best_axis = axis;
                        split_pos = pos;
                    }
                }
            }

            float nosplit_cost =
                (nodes[cur].tri_end - nodes[cur].tri_start + 1) * nodes[cur].aabb.area();
            if (best_axis == -1 || min_cost > nosplit_cost) {
                // no split
                continue;
            }

            int left_cnt = 0;
            for (int i = nodes[cur].tri_start; i <= nodes[cur].tri_end; i++) {
                const Triangle& tri = triangles[tri_indices[i]];
                if (get(tri.centroid, best_axis) < split_pos) {
                    left_cnt++;
                } else {
                    std::swap(tri_indices[i], tri_indices[nodes[cur].tri_end]);
                }
            }

            int left = nodes.size();
            int left_start = nodes[cur].tri_start, left_end = left_start + left_cnt - 1;
            nodes.push_back(BVHNode(-1, -1, left_start, left_end));
            int right = nodes.size();
            int right_start = left_end + 1, right_end = nodes[cur].tri_end;
            nodes.push_back(BVHNode(-1, -1, right_start, right_end));

            nodes[cur].left = left;
            nodes[cur].right = right;

            q.push_back(left);
            q.push_back(right);
        }

        // build all aabbs
        for (int i = nodes.size() - 1; i >= 0; i--) {
            nodes[i].aabb = AABB();
            if (nodes[i].left == -1 && nodes[i].right == -1) {
                // leaf
                for (int j = nodes[i].tri_start; j <= nodes[i].tri_end; j++) {
                    nodes[i].aabb.merge(triangles[tri_indices[j]].aabb);
                }
            } else {
                nodes[i].aabb.merge(nodes[nodes[i].left].aabb);
                nodes[i].aabb.merge(nodes[nodes[i].right].aabb);
            }
        }
    }
    size_t size() const {
        return nodes.size();
    }
};

bool load_shader(const std::string& filename, sf::Shader& shader) {
    if (!sf::Shader::isAvailable()) {
        // shaders are not available...
        std::cerr << "Shaders are not available" << '\n';
        return false;
    }
    if (!shader.loadFromFile(filename, sf::Shader::Fragment)) {
        std::cerr << "Failed to load shader" << '\n';
        return false;
    }
    return true;
}

int main() {
    const ivec2 resolution(600, 600);
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

    const int FPS = 1000;
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
                            camera.move(Camera::FORWARD, 2);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::S: {
                            camera.move(Camera::BACKWARD, 2);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::A: {
                            camera.move(Camera::LEFT, 2);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::D: {
                            camera.move(Camera::RIGHT, 2);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Space: {
                            camera.move(Camera::UP, 2);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Z: {
                            camera.move(Camera::DOWN, 2);
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
    std::cout << '\n';
    std::cout << "Saved output.png" << '\n';
}