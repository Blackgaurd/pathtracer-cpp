#include <iostream>

#include "pathtracer/pathtracer.h"

#define DEG2RAD M_PI / 180

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    Camera camera = Camera(vec3(1, 1, 1), vec3(-1, -1, -1), vec3(0, 1, 0), ivec2(1024, 1024), 60 * DEG2RAD, 1);
    BVH bvh;
    for (int i = 0; i < 100; i++) {
        vec3 v1 = vec3(rng.rand01(), rng.rand01(), rng.rand01());
        vec3 v2 = vec3(rng.rand01(), rng.rand01(), rng.rand01());
        vec3 v3 = vec3(rng.rand01(), rng.rand01(), rng.rand01());
        vec3 color = vec3(rng.rand01(), rng.rand01(), rng.rand01());
        bvh.add_triangle(Triangle(v1, v2, v3, Material(Material::EMIT, 0, color, 0)));
    }
    bvh.build();

    render_gpu(camera, bvh, 10, 3, ivec2(200, 200), argv[1]);
    render_cpu(camera, bvh, 10, 3, argv[1] + std::string(".png"));
}