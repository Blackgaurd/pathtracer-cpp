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
    vec3 red = {1, 0, 0}, blue = {0, 0, 1}, green = {0, 1, 0}, yellow = {1, 1, 0};
    objects.push_back(std::make_shared<sphere_t>(sphere_t({-2, -2, -5}, 1, red)));
    objects.push_back(std::make_shared<sphere_t>(sphere_t({2, -2, -5}, 1, blue)));
    objects.push_back(std::make_shared<sphere_t>(sphere_t({2, 2, -5}, 1, yellow)));
    objects.push_back(std::make_shared<sphere_t>(sphere_t({-2, 2, -5}, 1, green)));

    render(70 * M_PI / 180, {0, 0, 0}, {0, 0, -1}, {0, 1, 0}, 1, {0.8, 0.8, 0.8}, objects, out_image);
    out_image.write_png("out.png");
    out_image.write_ppm("out.ppm");
}