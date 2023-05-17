#pragma once

#include <SFML/Graphics.hpp>
#include <iomanip>
#include <ios>
#include <iostream>
#include <thread>
#include <filesystem>

#include "bvh.h"
#include "camera.h"
#include "image.h"
#include "linalg.h"
#include "shader.h"

#define SHIFT_BIAS 1e-4

struct Timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    Timer() = default;

    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    void reset() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    float seconds() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto mili = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        return mili.count() / 1000.0f;
    }
};

vec3 trace(const BVH& bvh, const vec3& ray_o, const vec3& ray_d, int depth) {
    if (depth == 0) return 0;

    float hit_t;
    int hit_idx = bvh.intersect(ray_o, ray_d, hit_t);
    if (hit_idx == -1) return 0;

    const Triangle& tri = bvh.triangles[hit_idx];
    if (tri.material.type == Material::EMIT) {
        return tri.material.emit_color;
    }

    vec3 hit_p = ray_o + ray_d * hit_t;
    vec3 hit_n = tri.normal(ray_d, hit_p);

    vec3 new_d = tri.material.reflected_dir(ray_d, hit_n);
    vec3 new_o = hit_p + hit_n * SHIFT_BIAS;

    vec3 rec_color = trace(bvh, new_o, new_d, depth - 1);
    vec3 emission = tri.material.emit_color;
    vec3 surface_color = tri.material.color;
    float cos_theta = hit_n.dot(new_d);

    // multiply by 2 to account for cosine
    return emission + 2 * rec_color * surface_color * cos_theta;
}
bool render_cpu(const Camera& camera, BVH& bvh, int samples, int depth,
                const std::string& filename) {
    if (bvh.empty()) {
        std::cerr << "No triangles in scene.\n";
        return false;
    }
    if (!bvh.built) {
        std::cerr << "Bounding volume heirarchy not built.\nBuilding...\n";
        bvh.build();
    }

    auto [width, height] = camera.res;
    Image image(camera.res);
    vec3 ray_o, ray_d;

    Timer timer;
    timer.start();
    std::cout << "Rendered: 0/" << height << " rows.";
    for (int h = 0; h < height; h++) {
        for (int w = 0; w < width; w++) {
            for (int s = 0; s < samples; s++) {
                camera.get_ray(w, h, ray_o, ray_d);
                image.get_pixel(w, h) += trace(bvh, ray_o, ray_d, depth);
            }
        }
        std::cout << "\rRendered: " << (h + 1) << '/' << height << " rows." << std::flush;
    }
    float seconds = timer.seconds();

    std::ios old_state(nullptr);
    old_state.copyfmt(std::cout);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nDone in " << seconds << " seconds.\nColor correcting...\n";
    std::cout.copyfmt(old_state);

    image /= samples;
    // gamma correct
    image.gamma_correct(2.2);
    image.save_png(filename);
    std::cout << "Saved to " << filename << '\n';

    return true;
}

int ceildiv(int a, int b) {
    return (a + b - 1) / b;
}
bool render_gpu(const Camera& camera, BVH& bvh, int samples, int depth, const ivec2& chunk_size,
                const std::string& filename) {
    if (bvh.empty()) {
        std::cerr << "No triangles in scene.\n";
        return false;
    }
    if (!bvh.built) {
        std::cerr << "Bounding volume heirarchy not built.\nBuilding...\n";
        bvh.build();
    }

    // render in chunks as to not crash the operating system
    int total_chunks = ceildiv(camera.res.x, chunk_size.x) * ceildiv(camera.res.y, chunk_size.y);
    int rendered_chunks = 0;
    PathtraceShader shader = PathtraceShader(camera, bvh, samples, depth);

    Timer timer;
    timer.start();
    std::cout << "Rendered: 0/" << total_chunks << " chunks.";
    for (int i = 0; i < camera.res.x; i += chunk_size.x) {
        for (int j = 0; j < camera.res.y; j += chunk_size.y) {
            ivec2 top_left = ivec2(i, j);
            ivec2 bottom_right = ivec2(i + chunk_size.x, j + chunk_size.y);
            shader.draw_rect(top_left, bottom_right);
            shader.finish();

            rendered_chunks++;
            std::cout << "\rRendered: " << rendered_chunks << '/' << total_chunks << " chunks."
                      << std::flush;
        }
    }
    float seconds = timer.seconds();

    std::ios old_state(nullptr);
    old_state.copyfmt(std::cout);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nDone in " << seconds << " seconds.\n";
    std::cout.copyfmt(old_state);

    shader.save_png(filename);
    std::cout << "Saved to " << filename << '\n';

    return true;
}

bool sf_load_shader(const std::string& filename, sf::Shader& shader) {
    if (!sf::Shader::isAvailable()) {
        std::cerr << "Shaders are not available\n";
        return false;
    }
    // cant think of a better way to define REALTIME
    const std::string realtime_shader =
        "#version 330 core\n#define REALTIME\n//" + std::string(FRAG_SOURCE);
    if (!shader.loadFromMemory(realtime_shader, sf::Shader::Fragment)) {
        std::cerr << "Failed to load shader\n";
        return false;
    }
    return true;
}
sf::Glsl::Vec3 sf_vec3(const vec3& v) {
    return sf::Glsl::Vec3(v.x, v.y, v.z);
}
sf::Glsl::Vec2 sf_vec2(const vec2& v) {
    return sf::Glsl::Vec2(v.x, v.y);
}
sf::Glsl::Ivec2 sf_ivec2(const ivec2& v) {
    return sf::Glsl::Ivec2(v.x, v.y);
}
void sf_set_uniform(sf::Shader& shader, const BVH& bvh) {
    // load triangle indices
    for (size_t i = 0; i < bvh.tri_idx.size(); i++) {
        std::string name = "tri_indices[" + std::to_string(i) + "]";
        shader.setUniform(name, bvh.tri_idx[i]);
    }

    // load triangles
    for (size_t i = 0; i < bvh.triangles.size(); i++) {
        const Triangle& tri = bvh.triangles[i];
        std::string name = "triangles[" + std::to_string(i) + "].";
        shader.setUniform(name + "v1", sf_vec3(tri.v1));
        shader.setUniform(name + "v2", sf_vec3(tri.v2));
        shader.setUniform(name + "v3", sf_vec3(tri.v3));

        name += "material.";
        shader.setUniform(name + "type", tri.material.type);
        shader.setUniform(name + "color", sf_vec3(tri.material.color));
        shader.setUniform(name + "emit_color", sf_vec3(tri.material.emit_color));
        shader.setUniform(name + "roughness", tri.material.roughness);
    }

    // load bvh nodes
    for (size_t i = 0; i < bvh.nodes.size(); i++) {
        const BVHNode& node = bvh.nodes[i];
        std::string name = "bvh_nodes[" + std::to_string(i) + "].";
        shader.setUniform(name + "aabb.lb", sf_vec3(node.aabb.lb));
        shader.setUniform(name + "aabb.rt", sf_vec3(node.aabb.rt));
        shader.setUniform(name + "left", node.left);
        shader.setUniform(name + "right", node.right);
        shader.setUniform(name + "tri_start", node.tri_start);
        shader.setUniform(name + "tri_end", node.tri_end);
    }
}
void sf_set_uniform(sf::Shader& shader, const Camera& camera) {
    shader.setUniform("camera.pos", sf_vec3(camera.pos));
    shader.setUniform("camera.res", sf_ivec2(camera.res));
    shader.setUniform("camera.v_res", sf_vec2(camera.v_res));
    shader.setUniform("camera.cell_size", camera.cell_size);
    shader.setUniform("camera.image_distance", camera.distance);
    shader.setUniform("camera.transform", sf::Glsl::Mat4(camera.transform.data()));
}
void render_realtime(const Camera& camera, BVH& bvh, int depth, int frame_samples, const std::string &screenshot_dir, int fps = 30,
                     bool accumulate = true, bool vsync = false) {
    if (bvh.empty()) {
        std::cerr << "No triangles in scene.\n";
        return;
    }
    if (!bvh.built) {
        std::cerr << "Bounding volume heirarchy not built.\nBuilding...\n";
        bvh.build();
    }
    Camera cam = camera;

    std::cout << "Loading shader...";
    sf::Shader shader;
    if (!sf_load_shader(FRAG_SOURCE, shader)) return;
    shader.setUniform("render_samples", frame_samples);
    shader.setUniform("render_depth", depth);
    sf_set_uniform(shader, bvh);
    std::cout << "\rShader loaded.   \n";

    std::cout << "Creating window and texture...";
    sf::RenderWindow window(sf::VideoMode(cam.res.x, cam.res.y), "Pathtracer");
    window.setVerticalSyncEnabled(vsync);
    window.setFramerateLimit(fps);
    const sf::RectangleShape rect(sf::Vector2f(cam.res.x, cam.res.y));
    sf::RenderTexture texture;
    texture.create(cam.res.x, cam.res.y);
    std::cout << "\rWindow and texture created.   \n";

    std::cout << R"instructions(
+-------------------------------+
| Realtime render controls:     |
| wasd       - move camera      |
| arrow keys - rotate camera    |
| space      - move camera up   |
| z          - move camera down |
|                               |
| r          - reset camera     |
| p          - save screenshot  |
|                               |
| escape     - exit             |
+-------------------------------+
)instructions";

    int frame = 0;
    const float rotate_angle = 5 * DEG2RAD;
    const float move_speed = 1;
    bool camera_changed = true;
    std::filesystem::path dir = screenshot_dir;
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
                        case sf::Keyboard::Left: {
                            cam.rotate(Camera::Direction::LEFT, rotate_angle);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Right: {
                            cam.rotate(Camera::Direction::RIGHT, rotate_angle);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Up: {
                            cam.rotate(Camera::Direction::UP, rotate_angle);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Down: {
                            cam.rotate(Camera::Direction::DOWN, rotate_angle);
                            camera_changed = true;
                            break;
                        }

                        case sf::Keyboard::W: {
                            cam.move(Camera::Direction::FORWARD, move_speed);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::S: {
                            cam.move(Camera::Direction::BACKWARD, move_speed);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::A: {
                            cam.move(Camera::Direction::LEFT, move_speed);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::D: {
                            cam.move(Camera::Direction::RIGHT, move_speed);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Space: {
                            cam.move(Camera::Direction::UP, move_speed);
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::Z: {
                            cam.move(Camera::Direction::DOWN, move_speed);
                            camera_changed = true;
                            break;
                        }

                        case sf::Keyboard::R: {
                            cam = camera;
                            camera_changed = true;
                            break;
                        }
                        case sf::Keyboard::P: {
                            std::string filename = dir / (std::to_string(frame) + ".png");
                            std::cout << "Saving screenshot to " << filename << '\n';

                            auto window_size = window.getSize();
                            sf::Texture screenshot;
                            screenshot.create(window_size.x, window_size.y);
                            screenshot.update(window);
                            sf::Image image = screenshot.copyToImage();

                            image.saveToFile(filename);
                            break;
                        }
                        default:
                            break;
                    }
                }
                default:
                    break;
            }
        }

        shader.setUniform("frame", frame++);
        shader.setUniform("prev_frame", texture.getTexture());

        if (camera_changed) {
            sf_set_uniform(shader, cam);
            texture.clear();
            frame = 0;
            camera_changed = false;
        }
        if (!accumulate) {
            texture.clear();
            frame = 0;
        }

        texture.draw(rect, &shader);
        texture.display();  // what does this even do?

        window.setTitle(std::to_string(cam.forward.x) + ", " + std::to_string(cam.forward.y) +
                        ", " + std::to_string(cam.forward.z));
        window.clear();
        window.draw(sf::Sprite(texture.getTexture()));
        window.display();
    }
}