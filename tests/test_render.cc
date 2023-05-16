#include <iostream>

#include "pathtracer/pathtracer.h"

#define DEG2RAD M_PI / 180

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    Camera camera = Camera(vec3(1.8, 1.8, 1.8), vec3(-1, -1, -1), vec3(0, 1, 0), ivec2(512, 512), 60 * DEG2RAD, 1);
    BVH bvh;
    bvh.add_triangle(Triangle(vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0), Material(Material::DIFFUSE, vec3(1, 1, 1), 0, 0)));
    bvh.add_triangle(Triangle(vec3(0, 0, 0), vec3(0, 0, 1), vec3(0, 1, 0), Material(Material::DIFFUSE, vec3(0, 1, 0), 0, 0)));
    bvh.add_triangle(Triangle(vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 0, 1), Material(Material::EMIT, vec3(0, 0, 1), 1, 0)));
    bvh.build();

    render_gpu(camera, bvh, 500, 5, ivec2(200, 200), argv[1] + std::string(".gpu.png"));
    render_cpu(camera, bvh, 500, 5, argv[1] + std::string(".cpu.png"));
}