#include <chrono>
#include <iomanip>
#include <ostream>
#include <memory>
#include <string>

#include "fpng/fpng.h"
#include "raytracer/camera.h"
#include "raytracer/light.h"
#include "raytracer/linalg.h"
#include "raytracer/object.h"
#include "raytracer/rng.h"
#include "raytracer/scene.h"

int main() {
    vec3 light_green = {0.5, 1, 0.5};
    vec3 gray = {0.2, 0.2, 0.2};

    Scene scene;
    scene.shadow_samples = 4;
    scene.indirect_samples = 2;
    scene.add_object<Sphere>(vec3{10, 0, -2.5}, 2, light_green);
    scene.add_object<Sphere>(vec3{10, 0, 2.5}, 2, color::white);
    scene.add_object<Sphere>(vec3{10, -10002, 0}, 10000, gray);
    scene.add_light<PointLight>(vec3{7, 3, 0}, 0.5, color::white, 1);

    float fov = 70 * M_PI / 180;
    Camera camera = Camera({1920, 1080}, fov, 1, {0, 0, 0}, {1, 0, 0}, {0, 1, 0});

    int num_frames = 100;
    for (int i = 1; i <= num_frames; i++) {
        std::cout << "Rendering frame " << i << " of " << num_frames << "\r" << std::flush;
        Image image(camera.res);
        scene.render(image, camera, 3);
        std::string filename = "output/output" + std::string(3 - std::to_string(i).length(), '0') + std::to_string(i) + ".png";
        image.write_png(filename);
    }
}