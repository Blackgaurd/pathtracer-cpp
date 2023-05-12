#pragma once

#include <SFML/Graphics.hpp>
#include <chrono>
#include <iomanip>
#include <ios>
#include <iostream>
#include <thread>

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

    vec3 hit_p = ray_o + ray_d + hit_t;
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

bool load_shader(sf::Shader& shader, const std::string& shader_code) {
    if (!sf::Shader::isAvailable()) {
        std::cerr << "Shaders are not available\n";
        return false;
    }
    if (!shader.loadFromMemory(shader_code, sf::Shader::Fragment)) {
        std::cerr << "Failed to load shader\n";
        return false;
    }
    return true;
}
void set_uniform(sf::Shader& shader, const std::string& name, float x) {
    shader.setUniform(name, x);
}
void set_uniform(sf::Shader& shader, const std::string& name, int x) {
    shader.setUniform(name, x);
}
void set_uniform(sf::Shader& shader, const std::string& name, const vec3& v3) {
    shader.setUniform(name, sf::Glsl::Vec3(v3.x, v3.y, v3.z));
}
void set_uniform(sf::Shader& shader, const std::string& name, const vec2& v2) {
    shader.setUniform(name, sf::Glsl::Vec2(v2.x, v2.y));
}
void set_uniform(sf::Shader& shader, const std::string& name, const ivec2& v2) {
    shader.setUniform(name, sf::Glsl::Ivec2(v2.x, v2.y));
}
void set_uniform(sf::Shader& shader, const Camera& camera) {
    set_uniform(shader, "camera.pos", camera.pos);
    set_uniform(shader, "camera.res", camera.res);
    set_uniform(shader, "camera.v_res", camera.v_res);
    set_uniform(shader, "camera.cell_size", camera.cell_size);
    set_uniform(shader, "camera.image_distance", camera.distance);
    shader.setUniform("camera.transform", sf::Glsl::Mat4(camera.transform.data()));
}
void set_uniform(sf::Shader& shader, const BVH& bvh) {
    // load triangle indices
    for (size_t i = 0; i < bvh.tri_idx.size(); i++) {
        std::string name = "tri_indices[" + std::to_string(i) + "]";
        shader.setUniform(name, bvh.tri_idx[i]);
    }

    // load the triangles
    for (size_t i = 0; i < bvh.triangles.size(); i++) {
        const Triangle& tri = bvh.triangles[i];
        std::string name = "triangles[" + std::to_string(i) + "].";
        set_uniform(shader, name + "v1", tri.v1);
        set_uniform(shader, name + "v2", tri.v2);
        set_uniform(shader, name + "v3", tri.v3);
        set_uniform(shader, name + "aabb.lb", tri.aabb.lb);
        set_uniform(shader, name + "aabb.rt", tri.aabb.rt);

        name += "material.";
        set_uniform(shader, name + "type", tri.material.type);
        set_uniform(shader, name + "color", tri.material.color);
        set_uniform(shader, name + "emit_color", tri.material.emit_color);
        set_uniform(shader, name + "roughness", tri.material.roughness);
    }

    // load bvh nodes
    for (size_t i = 0; i < bvh.nodes.size(); i++) {
        const BVHNode& node = bvh.nodes[i];
        std::string name = "bvh_nodes[" + std::to_string(i) + "].";
        set_uniform(shader, name + "left", node.left);
        set_uniform(shader, name + "right", node.right);
        set_uniform(shader, name + "tri_start", node.tri_start);
        set_uniform(shader, name + "tri_end", node.tri_end);
        set_uniform(shader, name + "aabb.lb", node.aabb.lb);
        set_uniform(shader, name + "aabb.rt", node.aabb.rt);
    }
}
bool render_gpu(const Camera& camera, BVH& bvh, int samples, int depth, const std::string& filename,
                int frame_samples = 10, int fps = 60) {
    // make sure this task takes less than 100ms as to not
    // make the operating system think the program is unresponsive
    // as it runs on the gpu

    // solution:
    // split up the image into a grid of cells
    // and render the grids seperately

    // it looks like the .draw() function only pushes a
    // command onto the gpu queue and returns immediately
    // and the rendering is done after all the .draw() calls
    // which is why the gpu is still blocking the main thread
    // after the .draw() calls

    if (bvh.empty()) {
        std::cerr << "No triangles in scene.\n";
        return false;
    }
    if (!bvh.built) {
        std::cerr << "Bounding volume heirarchy not built.\nBuilding...\n";
        bvh.build();
    }

    sf::Shader pathtracer;
    if (!load_shader(pathtracer, ONETIME_SHADER)) return false;

    set_uniform(pathtracer, "render_depth", depth);
    set_uniform(pathtracer, "render_samples", samples);
    set_uniform(pathtracer, camera);
    set_uniform(pathtracer, bvh);

    sf::RenderTexture texture1, texture2, texture3, texture4;
    texture1.create(camera.res.x, camera.res.y);
    texture2.create(camera.res.x, camera.res.y);
    texture3.create(camera.res.x, camera.res.y);
    texture4.create(camera.res.x, camera.res.y);
    sf::RectangleShape rect(sf::Vector2f(400, 400));

    pathtracer.setUniform("uniform_seed", 0);

    Timer timer;
    timer.start();
    texture1.clear();
    texture1.draw(rect, &pathtracer);
    texture1.display();
    texture1.resetGLStates();
    // std::this_thread::sleep_for(std::chrono::seconds(5));
    rect.move(400, 0);
    texture2.clear();
    texture2.draw(rect, &pathtracer);
    texture2.display();
    texture2.resetGLStates();
    // std::this_thread::sleep_for(std::chrono::seconds(5));
    rect.move(-400, 400);
    texture3.clear();
    texture3.draw(rect, &pathtracer);
    texture3.display();
    texture3.resetGLStates();
    // std::this_thread::sleep_for(std::chrono::seconds(5));
    rect.move(400, 0);
    texture4.clear();
    texture4.draw(rect, &pathtracer);
    texture4.display();
    texture4.resetGLStates();

    // texture.display();
    float seconds = timer.seconds();

    std::ios old_state(nullptr);
    old_state.copyfmt(std::cout);
    std::cout << std::fixed << std::setprecision(5);
    std::cout << "Done in " << seconds << " seconds.\n";
    std::cout.copyfmt(old_state);

    texture1.getTexture().copyToImage().saveToFile(filename + "1.png");
    texture2.getTexture().copyToImage().saveToFile(filename + "2.png");
    texture3.getTexture().copyToImage().saveToFile(filename + "3.png");
    texture4.getTexture().copyToImage().saveToFile(filename + "4.png");
    std::cout << "Saved to " << filename << '\n';

    return true;
}

void render_realtime(const Camera& camera, BVH& bvh, int depth, int frame_samples) {
    // todo
    // camera is passed as const so just take a copy of it
}