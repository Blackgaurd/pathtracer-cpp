#pragma once

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

void render_realtime(const Camera& camera, BVH& bvh, int depth, int frame_samples) {
    // todo
    // camera is passed as const so just take a copy of it
}