#include <ostream>

#include "pathtracer/pathtracer.h"

int main() {
    Scene scene;
    scene.load_obj("examples/cats/cats.obj", "examples/cats/");
    std::cout << "Loaded " << scene.objects.size() << " objects" << std::endl;

    Resolution res = {512, 512};
    vec3 look_from = vec3(-0.25, 2.7, 5), look_at = vec3(-0.25, 2.7, 1);

    Camera camera = Camera(res, 60 * M_PI / 180, 1, look_from, look_at, vec3(0, 1, 0));
    Image image(res);
    scene.render_threaded(camera, image, 5, 1000, 10);
    image.gamma_correct(2.2);
    image.save_png("examples/cats/cats.png");
}