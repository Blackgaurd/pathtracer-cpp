#include <iomanip>
#include <iostream>
#include <memory>
#include <random>

#include "fpng/fpng.h"
#include "raytracer/linalg.h"
#include "raytracer/object.h"
#include "raytracer/render.h"

int main() {
    image_t out_image(600, 480);

    std::vector<std::shared_ptr<object_t>> objects;
    objects.push_back(std::make_shared<sphere_t>(sphere_t({-2, -2, -5}, 1, color::red)));
    objects.push_back(std::make_shared<sphere_t>(sphere_t({2, -2, -5}, 1, color::blue)));
    objects.push_back(std::make_shared<sphere_t>(sphere_t({2, 2, -5}, 1, color::yellow)));
    objects.push_back(std::make_shared<sphere_t>(sphere_t({-2, 2, -5}, 1, color::green)));

    std::vector<std::shared_ptr<light_t>> lights;
    lights.push_back(std::make_shared<dir_light_t>(dir_light_t({0, 0, -1}, color::white, 1)));

    render(70 * M_PI / 180, {0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 1, {0.8, 0.8, 0.8}, objects, lights, out_image);
    out_image.write_png("output/out.png");
    out_image.write_ppm("output/out.ppm");
}