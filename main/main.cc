#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>

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
    scene.add_object<Sphere>(vec3{10, 0, -3}, 2, light_green);
    scene.add_object<Sphere>(vec3{10, 0, 3}, 2, color::white);
    scene.add_object<Sphere>(vec3{10, -10002, 0}, 10000, gray);
    scene.add_light<PointLight>(vec3{7, 3, 0}, 0.5, color::white, 1);

    float fov = 70 * M_PI / 180;
    Camera camera = Camera({600, 480}, fov, 1, {0, 0, 0}, {1, 0, 0}, {0, 1, 0});
    Image image = Image(camera.res);

    scene.render(image, camera, 3);
    image.write_png("output2.png");
    //image.write_ppm("output.ppm");
}