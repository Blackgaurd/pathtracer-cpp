#include <iostream>

#include "pathtracer/pathtracer.h"

#define DEG2RAD M_PI / 180

int main() {
    BVH bvh;
    for (int i = 0; i < 200; i++) {
        vec3 v1 = vec3(rng.rand01(), rng.rand01(), rng.rand01()) * 10;
        vec3 v2 = vec3(rng.rand01(), rng.rand01(), rng.rand01()) * 10;
        vec3 v3 = vec3(rng.rand01(), rng.rand01(), rng.rand01()) * 10;
        vec3 color = vec3(rng.rand01(), rng.rand01(), rng.rand01());
        Triangle tri = Triangle(v1, v2, v3, Material(Material::EMIT, 0, color, 0));
        bvh.add_triangle(tri);
    }
    bvh.build();

    Camera camera(vec3(15, 15, 15), vec3(-1, -1, -1), vec3(0, 1, 0), ivec2(800, 800), 60 * DEG2RAD, 1);
    //std::cout << "rendering on cpu...\n";
    //render_cpu(camera, bvh, 500, 5, "cpu_test.png");
    std::cout << "rendering on gpu...\n";
    render_gpu(camera, bvh, 5000, 5, "gpu_test.png", 30, 2);
}