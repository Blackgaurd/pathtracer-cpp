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

    Scene scene;
    //scene.add_object<Sphere>(vec3(0, 0, 0), 5, yellow_specular);
    scene.add_object<Sphere>(vec3(0, -10005, 0), 10000, white_diffuse);
    scene.add_object<Sphere>(vec3(10, 10, 2), 5, white_emit);
    scene.add_object<Triangle>(vec3(4, 0, 0), vec3(0, 4, 0), vec3(0, 0, 4),
                               red_diffuse);

    vec3 look_from = vec3(20, 5, 0), look_at = vec3(0, 0, 0);
    Resolution res = {600, 480};
    Camera camera =
        Camera(res, 70 * M_PI / 180, 1, look_from, look_at, vec3(0, 1, 0));

    Image image = Image(res);
    scene.render(camera, image, 3, 1000);

    image.gamma_correct(2.2);
    image.write_png("test3.png");
}