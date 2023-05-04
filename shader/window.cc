#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <deque>
#include <iomanip>
#include <iostream>

#include "sfml_linalg.h"

#define WATCH(x) std::cerr << #x << ": " << x << '\n'

// #define V_SYNC

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

std::string vec3_str(const vec3& v, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << v;
    return ss.str();
}
std::string to_string(const vec3& v) {
    return vec3_str(v, 2);
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
        this->forward = normalize(forward);
        this->right = normalize(cross(forward, up));
        this->up = normalize(cross(right, forward));
        this->world_up = this->up;

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
                right = normalize(cross(forward, world_up));
                up = normalize(cross(right, forward));
                break;
            }
            case RIGHT: {
                forward = normalize(forward * std::cos(angle) + right * std::sin(angle));
                right = normalize(cross(forward, world_up));
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
                vec3 const_forward = normalize(cross(world_up, right));
                pos += const_forward * amount;
                break;
            }
            case BACKWARD: {
                vec3 const_forward = normalize(cross(world_up, right));
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
vec3 component_max(const vec3& a, const vec3& b) {
    return vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
}
struct AABB {
    vec3 lb = vec3_from(FLOAT_INF), rt = vec3_from(-FLOAT_INF);

    AABB() = default;
    AABB(const vec3& lb, const vec3& rt) : lb(lb), rt(rt) {}

    void merge(const AABB& other) {
        lb = component_min(lb, other.lb);
        rt = component_max(rt, other.rt);
    }
    void merge(const vec3& point) {
        lb = component_min(lb, point);
        rt = component_max(rt, point);
    }
    bool is_valid() const {
        return lb.x <= rt.x && lb.y <= rt.y && lb.z <= rt.z;
    }
    float area() const {
        // half of surface area
        if (!is_valid()) throw std::runtime_error("invalid AABB");
        vec3 diff = rt - lb;
        return diff.x * diff.y + diff.x * diff.z + diff.y * diff.z;
    }

    friend std::ostream& operator<<(std::ostream& os, const AABB& aabb) {
        os << "AABB: " << aabb.lb << ' ' << aabb.rt;
        return os;
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

    bool is_leaf() const {
        return left == -1 && right == -1;
    }
};
std::ostream& operator<<(std::ostream& os, std::vector<int> const& v) {
    os << '[';
    for (int i = 0; i < int(v.size()); i++) {
        os << v[i];
        if (i != int(v.size()) - 1) os << ", ";
    }
    os << ']';
    return os;
}
struct BVH {
    std::vector<Triangle> triangles;
    std::vector<int> tri_idx;
    std::vector<BVHNode> nodes;

    BVH() = default;
    BVH(const std::vector<Triangle>& triangles) {
        this->triangles = triangles;
        // todo: try replacing stack with queue
        build_stack();
    }

    void add_tri(const Triangle& tri) {
        triangles.push_back(tri);
    }

    void build_stack() {
        // stack based bvh building
        tri_idx.resize(triangles.size());
        for (int i = 0; i < int(tri_idx.size()); i++) tri_idx[i] = i;

        nodes.reserve(triangles.size() * 2);
        nodes.push_back(BVHNode(-1, -1, 0, triangles.size() - 1));

        std::deque<int> q;
        q.push_back(0);
        while (!q.empty()) {
            BVHNode& cur = nodes[q.back()];
            q.pop_back();

            // build current aabb
            for (int i = cur.tri_start; i <= cur.tri_end; i++) {
                cur.aabb.merge(triangles[tri_idx[i]].aabb);
            }

            // todo: extract finding best split position and axis to a function
            int best_axis = -1;
            float split_pos = 0, min_cost = FLOAT_INF;
            for (int axis = 0; axis < 3; axis++) {
                for (int i = cur.tri_start; i <= cur.tri_end; i++) {
                    const Triangle& tri = triangles[tri_idx[i]];
                    float pos = get(tri.centroid, axis);
                    int left_cnt = 0, right_cnt = 0;
                    AABB left_aabb, right_aabb;
                    for (int j = cur.tri_start; j <= cur.tri_end; j++) {
                        const Triangle& other = triangles[tri_idx[j]];
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

            int tri_count = cur.tri_end - cur.tri_start + 1;
            float nosplit_cost = tri_count * cur.aabb.area();
            if (best_axis == -1 || min_cost > nosplit_cost) {
                // no split
                continue;
            }

            // triangle "sorting" algorithm:
            // two pointer method
            // "array of centroids" below
            // e.g. [9, 7, 3, 6, 3, 5, 2, 9, 4, 10] with split_pos = 7
            // should become [3, 6, 3, 5, 2, 4, 9, 7, 9, 10]
            //                      split here ^
            // - two pointers, one at the start, one at the end
            // - advance start pointer until element at index >= 7 (split_pos)
            // - move back end pointer until element at index < 7 (split_pos)
            // - swap the two elements
            // - repeat until start pointer >= end pointer
            // - count the number of elements on the left (left_cnt = 6)

            int start = cur.tri_start, end = cur.tri_end;
            int left_cnt = 0;
            while (start < end) {
                if (get(triangles[tri_idx[start]].centroid, best_axis) < split_pos) {
                    start++;
                    left_cnt++;
                } else if (get(triangles[tri_idx[end]].centroid, best_axis) >= split_pos) {
                    end--;
                } else {
                    std::swap(tri_idx[start], tri_idx[end]);
                }
            }

            if (left_cnt == 0 || left_cnt == tri_count) {
                // no split
                continue;
            }
            int left = nodes.size();
            int left_start = cur.tri_start, left_end = left_start + left_cnt - 1;
            nodes.push_back(BVHNode(-1, -1, left_start, left_end));
            cur.left = left;

            int right = nodes.size();
            int right_start = cur.tri_start + left_cnt, right_end = cur.tri_end;
            nodes.push_back(BVHNode(-1, -1, right_start, right_end));
            cur.right = right;

            q.push_back(left);
            q.push_back(right);
        }
    }

    void set_uniform(sf::Shader& shader) const {
        // load triangle indices
        for (int i = 0; i < int(tri_idx.size()); i++) {
            std::string name = "tri_indices[" + std::to_string(i) + "]";
            shader.setUniform(name, tri_idx[i]);
        }

        // load the triangles
        for (int i = 0; i < int(triangles.size()); i++) {
            const Triangle& tri = triangles[i];
            std::string name = "triangles[" + std::to_string(i) + "].";
            shader.setUniform(name + "v1", tri.v1);
            shader.setUniform(name + "v2", tri.v2);
            shader.setUniform(name + "v3", tri.v3);

            shader.setUniform(name + "aabb.lb", tri.aabb.lb);
            shader.setUniform(name + "aabb.rt", tri.aabb.rt);

            name += "material.";
            shader.setUniform(name + "type", tri.material.type);
            shader.setUniform(name + "color", tri.material.color);
            shader.setUniform(name + "emit_color", tri.material.emit_color);
            shader.setUniform(name + "roughness", tri.material.roughness);
        }

        // load bvh nodes
        for (int i = 0; i < int(nodes.size()); i++) {
            const BVHNode& node = nodes[i];
            std::string name = "bvh_nodes[" + std::to_string(i) + "].";
            shader.setUniform(name + "left", node.left);
            shader.setUniform(name + "right", node.right);
            shader.setUniform(name + "tri_start", node.tri_start);
            shader.setUniform(name + "tri_end", node.tri_end);
            shader.setUniform(name + "aabb.lb", node.aabb.lb);
            shader.setUniform(name + "aabb.rt", node.aabb.rt);
        }
    }

    void print(int node_idx = 0, int depth = 0, std::string dir = "root") const {
        if (node_idx == -1) return;
        std::cout << node_idx << ": ";
        for (int i = 0; i < depth; i++) std::cout << " | ";
        if (depth > 0) std::cout << " +-";

        const BVHNode& node = nodes[node_idx];
        std::cout << node.aabb.lb << ' ' << node.aabb.rt;
        if (node.is_leaf()) {
            std::cout << " leaf, tri: " << node.tri_start << " -> " << node.tri_end;
            std::cout << " (" << dir << ")\n";
        } else {
            std::cout << " tri: " << node.tri_start << " -> " << node.tri_end;
            std::cout << " (" << dir << ")\n";
            print(node.left, depth + 1, "left");
            print(node.right, depth + 1, "right");
        }
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
    const ivec2 resolution(800, 800);

    sf::Shader pathtracer;
    if (!load_shader("shader/pathtracer.frag", pathtracer)) return 1;

    vec3 pos = {278, 278, -500}, forward = {0, 0, 1}, up = {0, 1, 0};
    Camera camera(pos, forward, up, resolution, 60 * DEG2RAD, 1);

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

    BVH bvh(triangles);
    bvh.set_uniform(pathtracer);
    bvh.print();

    const int FPS = 30;
    sf::RenderWindow window(sf::VideoMode(resolution.x, resolution.y), "");
    window.setFramerateLimit(FPS);
#ifdef V_SYNC
    window.setVerticalSyncEnabled(true);
#endif
    const sf::RectangleShape screen(sf::Vector2f(resolution.x, resolution.y));
    pathtracer.setUniform("resolution", camera.res);

    int frame = 1;
    int frame_count = 0;
    sf::RenderTexture texture;
    texture.create(resolution.x, resolution.y);

    std::cout << std::fixed << std::setprecision(2);
    auto prev_time = std::chrono::high_resolution_clock::now();
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

        std::string title = "pos: " + to_string(camera.pos) +
                            " | forward: " + to_string(camera.forward) +
                            " | up: " + to_string(camera.up);
        window.setTitle(title);
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