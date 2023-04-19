#include "bvh.h"

#include <ostream>
#include <iomanip>

float random_float(float a, float b) {
    return a + (b - a) * (float)rand() / RAND_MAX;
}
int main() {
    auto white_diffuse = std::make_shared<Diffuse>(vec3(1, 1, 1));
    std::vector<std::shared_ptr<Object>> objects;
    std::cout << std::fixed << std::setprecision(2);
    for (int i = 0; i < 2242324; i++) {
        vec3 v1 = vec3(random_float(-10, 10), random_float(-10, 10),
                       random_float(-10, 10));
        vec3 v2 = vec3(random_float(-10, 10), random_float(-10, 10),
                       random_float(-10, 10));
        vec3 v3 = vec3(random_float(-10, 10), random_float(-10, 10),
                       random_float(-10, 10));
        objects.push_back(
            std::make_shared<Triangle>(v1, v2, v3, white_diffuse));
    }

    std::cout << sizeof(objects) << std::endl;

    BVHNodePtr root = build_bvh(objects);
    std::cout << root->left->is_leaf << std::endl;
}