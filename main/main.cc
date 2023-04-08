#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>

#include "fpng/fpng.h"
#include "raytracer/linalg.h"
#include "raytracer/object.h"
#include "raytracer/render.h"
#include "raytracer/rng.h"

int main() {
    image_t out_image(600, 480);

    std::vector<std::shared_ptr<object_t>> objects;
    objects.push_back(std::make_shared<sphere_t>(sphere_t({0, 0, 0}, 2, color::mix(color::red, color::green, 0.5))));
    objects.push_back(std::make_shared<sphere_t>(sphere_t({0, -10002, 0}, 10000, color::white)));

    std::vector<std::shared_ptr<light_t>> lights;
    lights.push_back(std::make_shared<point_light_t>(point_light_t({-4, 0.5, 2}, 5, color::white, 1)));
    lights.push_back(std::make_shared<point_light_t>(point_light_t({1, 3, -2}, 1, color::white, 1)));

    vec3 look_from = {-10, 0, 0};
    vec3 look_at = {-9, 0, 0};
    vec3 bg_color = color::black;

    render(70 * M_PI / 180, look_from, look_at, {0, 1, 0}, 1, bg_color, objects, lights, out_image);
    out_image.write_png("out5.png");
}