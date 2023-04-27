#include <string>

#include "pathtracer/pathtracer.h"

int main() {
    MaterialPtr red_diffuse = std::make_shared<Diffuse>(vec3(1, 0, 0));
    MaterialPtr white_diffuse = std::make_shared<Diffuse>(vec3(1, 1, 1));
    MaterialPtr green_diffuse = std::make_shared<Diffuse>(vec3(0, 1, 0));
    MaterialPtr white_emit = std::make_shared<Emit>(vec3(1, 1, 1));

    Scene scene;
    // floor
    vec3 f1 = vec3(552.8, 0, 0), f2 = vec3(0, 0, 0), f3 = vec3(0, 0, 559.2),
         f4 = vec3(549.6, 0, 559.2);
    scene.add_object<Triangle>(f1, f2, f3, white_diffuse);
    scene.add_object<Triangle>(f1, f3, f4, white_diffuse);

    // light
    vec3 l1 = vec3(343, 548.7, 227), l2 = vec3(343, 548.7, 332), l3 = vec3(213, 548.7, 332),
         l4 = vec3(213, 548.7, 227);
    scene.add_object<Triangle>(l1, l2, l3, white_emit);
    scene.add_object<Triangle>(l1, l3, l4, white_emit);

    // ceiling
    vec3 c1 = vec3(556, 548.8, 0), c2 = vec3(0, 548.8, 0), c3 = vec3(0, 548.8, 559.2),
         c4 = vec3(556.0, 548.8, 559.2);
    scene.add_object<Triangle>(c1, c2, c3, white_diffuse);
    scene.add_object<Triangle>(c1, c3, c4, white_diffuse);

    // back wall
    vec3 b1 = vec3(549.6, 0, 559.2), b2 = vec3(0, 0, 559.2), b3 = vec3(0, 548.8, 559.2),
         b4 = vec3(556, 548.8, 559.2);
    scene.add_object<Triangle>(b1, b2, b3, white_diffuse);
    scene.add_object<Triangle>(b1, b3, b4, white_diffuse);

    // right wall
    vec3 r1 = vec3(0, 0, 559.2), r2 = vec3(0, 0, 0), r3 = vec3(0, 548.8, 0),
         r4 = vec3(0, 548.8, 559.2);
    scene.add_object<Triangle>(r1, r2, r3, green_diffuse);
    scene.add_object<Triangle>(r1, r3, r4, green_diffuse);

    // left wall
    vec3 lw1 = vec3(552.8, 0, 0), lw2 = vec3(549.6, 0, 559.2), lw3 = vec3(556, 548.8, 559.2),
         lw4 = vec3(556, 548.8, 0);
    scene.add_object<Triangle>(lw1, lw2, lw3, red_diffuse);
    scene.add_object<Triangle>(lw1, lw3, lw4, red_diffuse);

    // short block
    vec3 sb1 = vec3(130, 165, 65), sb2 = vec3(82, 165, 225), sb3 = vec3(240, 165, 272),
         sb4 = vec3(290, 165, 114);
    scene.add_object<Triangle>(sb1, sb2, sb3, white_diffuse);
    scene.add_object<Triangle>(sb1, sb3, sb4, white_diffuse);
    vec3 sb5 = vec3(290, 0, 114), sb6 = vec3(290, 165, 114), sb7 = vec3(240, 165, 272),
         sb8 = vec3(240, 0, 272);
    scene.add_object<Triangle>(sb5, sb6, sb7, white_diffuse);
    scene.add_object<Triangle>(sb5, sb7, sb8, white_diffuse);
    vec3 sb9 = vec3(130, 0, 65), sb10 = vec3(130, 165, 65), sb11 = vec3(290, 165, 114),
         sb12 = vec3(290, 0, 114);
    scene.add_object<Triangle>(sb9, sb10, sb11, white_diffuse);
    scene.add_object<Triangle>(sb9, sb11, sb12, white_diffuse);
    vec3 sb13 = vec3(82, 0, 225), sb14 = vec3(82, 165, 225), sb15 = vec3(130, 165, 65),
         sb16 = vec3(130, 0, 65);
    scene.add_object<Triangle>(sb13, sb14, sb15, white_diffuse);
    scene.add_object<Triangle>(sb13, sb15, sb16, white_diffuse);
    vec3 sb17 = vec3(240, 0, 272), sb18 = vec3(240, 165, 272), sb19 = vec3(82, 165, 225),
         sb20 = vec3(82, 0, 225);
    scene.add_object<Triangle>(sb17, sb18, sb19, white_diffuse);
    scene.add_object<Triangle>(sb17, sb19, sb20, white_diffuse);

    // tall block
    vec3 tb1 = vec3(423, 330, 247), tb2 = vec3(265, 330, 296), tb3 = vec3(314, 330, 456),
         tb4 = vec3(472, 330, 406);
    scene.add_object<Triangle>(tb1, tb2, tb3, white_diffuse);
    scene.add_object<Triangle>(tb1, tb3, tb4, white_diffuse);
    vec3 tb5 = vec3(423, 0, 247), tb6 = vec3(423, 330, 247), tb7 = vec3(472, 330, 406),
         tb8 = vec3(472, 0, 406);
    scene.add_object<Triangle>(tb5, tb6, tb7, white_diffuse);
    scene.add_object<Triangle>(tb5, tb7, tb8, white_diffuse);
    vec3 tb9 = vec3(472, 0, 406), tb10 = vec3(472, 330, 406), tb11 = vec3(314, 330, 456),
         tb12 = vec3(314, 0, 456);
    scene.add_object<Triangle>(tb9, tb10, tb11, white_diffuse);
    scene.add_object<Triangle>(tb9, tb11, tb12, white_diffuse);
    vec3 tb13 = vec3(314, 0, 456), tb14 = vec3(314, 330, 456), tb15 = vec3(265, 330, 296),
         tb16 = vec3(265, 0, 296);
    scene.add_object<Triangle>(tb13, tb14, tb15, white_diffuse);
    scene.add_object<Triangle>(tb13, tb15, tb16, white_diffuse);
    vec3 tb17 = vec3(265, 0, 296), tb18 = vec3(265, 330, 296), tb19 = vec3(423, 330, 247),
         tb20 = vec3(423, 0, 247);
    scene.add_object<Triangle>(tb17, tb18, tb19, white_diffuse);
    scene.add_object<Triangle>(tb17, tb19, tb20, white_diffuse);

    vec3 look_from = vec3(278, 278, -500), look_at = vec3(278, 278, 0), up = vec3(0, 1, 0);
    Resolution res = Resolution(512, 512);
    Camera camera = Camera(res, 60 * M_PI / 180, 1, look_from, look_at, up);
    Image image = Image(res);

    scene.render_threaded(camera, image, 5, 100, 10);
    image.gamma_correct(2.2);

    image.save_png("examples/cornell/cornell_box.png");
}