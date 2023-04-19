#include <iostream>
#include <memory>

#include "camera.h"
#include "image.h"
#include "linalg.h"
#include "material.h"
#include "object.h"
#include "scene.h"

int main() {
    auto red_diffuse = std::make_shared<Diffuse>(vec3(1, 0, 0));
    auto white_diffuse = std::make_shared<Diffuse>(vec3(1, 1, 1));
    auto green_emit = std::make_shared<Emit>(vec3(0, 1, 0));
    auto white_emit = std::make_shared<Emit>(vec3(1, 1, 1));
    auto yellow_specular = std::make_shared<Specular>(vec3(1, 1, 0), 0.3);
    auto white_specular = std::make_shared<Specular>(vec3(1, 1, 1), 0.2);

    Scene scene;
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;
    materials["(null)"] = white_diffuse;
    scene.load_obj("pathtracer/torus.obj", materials);
    //scene.add_object<Sphere>(vec3(0, -100, 0), 98, white_diffuse);
    scene.add_object<Sphere>(vec3(2.5, 1, 0.5), 0.3, green_emit);

    vec3 look_from = vec3(4, 3, 2), look_at = vec3(2.5, 1, 0.5);

    Resolution res(600, 480);
    Camera camera(res, 70 * M_PI / 180, 1, look_from, look_at, vec3(0, 1, 0));
    Image image(res);
    scene.render(camera, image, 3, 100);
    image.write_png("pathtracer/test_scene.png");
}