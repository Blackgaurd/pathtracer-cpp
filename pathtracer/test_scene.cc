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
    auto green_diffuse = std::make_shared<Diffuse>(vec3(0, 1, 0));
    auto blue_diffuse = std::make_shared<Diffuse>(vec3(0, 0, 1));
    auto yellow_diffuse = std::make_shared<Diffuse>(vec3(1, 1, 0));
    auto white_emit = std::make_shared<Emit>(vec3(1, 1, 1));
    auto red_emit = std::make_shared<Emit>(vec3(1, 0, 0));
    auto blue_emit = std::make_shared<Emit>(vec3(0, 0, 1));
    auto yellow_emit = std::make_shared<Emit>(vec3(1, 1, 0));
    auto yellow_specular = std::make_shared<Specular>(vec3(1, 1, 0), 0.3);
    auto white_specular = std::make_shared<Specular>(vec3(1, 1, 1), 0.2);

    Scene scene;
    // floor
    vec3 f1 = vec3(552.8, 0, 0), f2 = vec3(0, 0, 0), f3 = vec3(0, 0, 559.2),
         f4 = vec3(549.6, 0, 559.2);
    scene.add_object<Triangle>(f1, f2, f3, blue_emit);
    scene.add_object<Triangle>(f1, f3, f4, blue_emit);

    // light
    vec3 l1 = vec3(343, 548.8, 227), l2 = vec3(343, 548.8, 332), l3 = vec3(213, 548.8, 332),
         l4 = vec3(213, 548.8, 227);
    scene.add_object<Triangle>(l1, l2, l3, white_emit);
    scene.add_object<Triangle>(l1, l3, l4, white_emit);

    // ceiling
    vec3 c1 = vec3(556, 548.8, 0), c2 = vec3(0, 548.8, 0), c3 = vec3(0, 548.8, 559.2),
         c4 = vec3(549.6, 548.8, 559.2);
    scene.add_object<Triangle>(c1, c2, c3, white_emit);
    scene.add_object<Triangle>(c1, c3, c4, white_emit);

    // back wall
    vec3 b1 = vec3(549.6, 0, 559.2), b2 = vec3(0, 0, 559.2), b3 = vec3(0, 548.8, 559.2),
         b4 = vec3(556, 548.8, 559.2);
    scene.add_object<Triangle>(b1, b2, b3, yellow_emit);
    scene.add_object<Triangle>(b1, b3, b4, yellow_emit);

    // right wall
    vec3 r1 = vec3(0, 0, 559.2), r2 = vec3(0, 0, 0), r3 = vec3(0, 548.8, 0),
         r4 = vec3(0, 548.8, 559.2);
    scene.add_object<Triangle>(r1, r2, r3, green_emit);
    scene.add_object<Triangle>(r1, r3, r4, green_emit);

    // left wall
    vec3 lw1 = vec3(552.8, 0, 0), lw2 = vec3(549.6, 0, 559.2), lw3 = vec3(556, 548.8, 559.2),
         lw4 = vec3(556, 548.8, 0);
    scene.add_object<Triangle>(lw1, lw2, lw3, red_emit);
    scene.add_object<Triangle>(lw1, lw3, lw4, red_emit);

    // short block
    vec3 sb1 = vec3(130, 165, 65), sb2 = vec3(82, 165, 225), sb3 = vec3(240, 165, 272),
         sb4 = vec3(290, 165, 114);
    scene.add_object<Triangle>(sb1, sb2, sb3, white_emit);
    scene.add_object<Triangle>(sb1, sb3, sb4, white_emit);
    vec3 sb5 = vec3(290, 0, 114), sb6 = vec3(290, 165, 114), sb7 = vec3(240, 165, 272),
         sb8 = vec3(240, 0, 272);
    scene.add_object<Triangle>(sb5, sb6, sb7, white_emit);
    scene.add_object<Triangle>(sb5, sb7, sb8, white_emit);
    vec3 sb9 = vec3(130, 0, 65), sb10 = vec3(130, 165, 65), sb11 = vec3(290, 165, 114),
         sb12 = vec3(290, 0, 114);
    scene.add_object<Triangle>(sb9, sb10, sb11, white_emit);
    scene.add_object<Triangle>(sb9, sb11, sb12, white_emit);
    vec3 sb13 = vec3(82, 0, 225), sb14 = vec3(82, 165, 225), sb15 = vec3(130, 165, 65),
         sb16 = vec3(130, 0, 65);
    scene.add_object<Triangle>(sb13, sb14, sb15, white_emit);
    scene.add_object<Triangle>(sb13, sb15, sb16, white_emit);
    vec3 sb17 = vec3(240, 0, 272), sb18 = vec3(240, 165, 272), sb19 = vec3(82, 165, 225),
         sb20 = vec3(82, 0, 225);
    scene.add_object<Triangle>(sb17, sb18, sb19, white_emit);
    scene.add_object<Triangle>(sb17, sb19, sb20, white_emit);

    // tall block
    vec3 tb1 = vec3(423, 330, 247), tb2 = vec3(265, 330, 296), tb3 = vec3(314, 330, 456),
         tb4 = vec3(472, 330, 406);
    scene.add_object<Triangle>(tb1, tb2, tb3, green_emit);
    scene.add_object<Triangle>(tb1, tb3, tb4, green_emit);
    vec3 tb5 = vec3(423, 0, 247), tb6 = vec3(423, 330, 247), tb7 = vec3(472, 330, 406),
         tb8 = vec3(472, 0, 406);
    scene.add_object<Triangle>(tb5, tb6, tb7, white_emit);
    scene.add_object<Triangle>(tb5, tb7, tb8, white_emit);
    vec3 tb9 = vec3(472, 0, 406), tb10 = vec3(472, 330, 406), tb11 = vec3(314, 330, 456),
         tb12 = vec3(314, 0, 456);
    scene.add_object<Triangle>(tb9, tb10, tb11, red_emit);  // right leaf
    scene.add_object<Triangle>(tb9, tb11, tb12, red_emit);
    vec3 tb13 = vec3(314, 0, 456), tb14 = vec3(314, 330, 456), tb15 = vec3(265, 330, 296),
         tb16 = vec3(265, 0, 296);
    scene.add_object<Triangle>(tb13, tb14, tb15, blue_emit);
    scene.add_object<Triangle>(tb13, tb15, tb16, blue_emit);
    vec3 tb17 = vec3(265, 0, 296), tb18 = vec3(265, 330, 296), tb19 = vec3(423, 330, 247),
         tb20 = vec3(423, 0, 247);
    scene.add_object<Triangle>(tb17, tb18, tb19, white_emit);
    scene.add_object<Triangle>(tb17, tb19, tb20, white_emit);  // left leaf

    vec3 look_from = vec3(278, 278, -600), look_at = vec3(278, 278, 0), up = vec3(0, 1, 0);
    Resolution res = Resolution(600, 480);
    Camera camera = Camera(res, 70.f * M_PI / 180, 1, look_from, look_at, up);
    Image image = Image(res);

    scene.render(camera, image, 1, 1000);
    image.gamma_correct(2.2);
    image.write_png("test_bvh.png");
}