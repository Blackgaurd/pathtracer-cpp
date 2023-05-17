#include <cstdio>

#include "pathtracer/pathtracer.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    Material white_diffuse = Material(Material::DIFFUSE, 1, 0, 0);
    Material white_emit = Material(Material::EMIT, 0, 1, 0);
    Material green_diffuse = Material(Material::DIFFUSE, vec3(0, 1, 0), 0, 0);
    Material red_diffuse = Material(Material::DIFFUSE, vec3(1, 0, 0), 0, 0);

    BVH bvh;
	// floor
    vec3 f1 = vec3(552.8, 0, 0), f2 = vec3(0, 0, 0), f3 = vec3(0, 0, 559.2),
         f4 = vec3(549.6, 0, 559.2);
    bvh.add_triangle(Triangle(f1, f2, f3, white_diffuse));
    bvh.add_triangle(Triangle(f4, f3, f1, white_diffuse));

	// light
    vec3 l1 = vec3(343, 548.7, 227), l2 = vec3(343, 548.7, 332), l3 = vec3(213, 548.7, 332),
         l4 = vec3(213, 548.7, 227);
    bvh.add_triangle(Triangle(l1, l2, l3, white_emit));
    bvh.add_triangle(Triangle(l4, l3, l1, white_emit));

	// ceiling
    vec3 c1 = vec3(556, 548.8, 0), c2 = vec3(0, 548.8, 0), c3 = vec3(0, 548.8, 559.2),
         c4 = vec3(556.0, 548.8, 559.2);
    bvh.add_triangle(Triangle(c1, c2, c3, white_diffuse));
    bvh.add_triangle(Triangle(c4, c3, c1, white_diffuse));

	// back wall
    vec3 b1 = vec3(549.6, 0, 559.2), b2 = vec3(0, 0, 559.2), b3 = vec3(0, 548.8, 559.2),
         b4 = vec3(556, 548.8, 559.2);
    bvh.add_triangle(Triangle(b1, b2, b3, white_diffuse));
    bvh.add_triangle(Triangle(b4, b3, b1, white_diffuse));

	// right wall
    vec3 r1 = vec3(0, 0, 559.2), r2 = vec3(0, 0, 0), r3 = vec3(0, 548.8, 0),
         r4 = vec3(0, 548.8, 559.2);
    bvh.add_triangle(Triangle(r1, r2, r3, green_diffuse));
    bvh.add_triangle(Triangle(r4, r3, r1, green_diffuse));

	// left wall
    vec3 lw1 = vec3(552.8, 0, 0), lw2 = vec3(549.6, 0, 559.2), lw3 = vec3(556, 548.8, 559.2),
         lw4 = vec3(556, 548.8, 0);
    bvh.add_triangle(Triangle(lw1, lw2, lw3, red_diffuse));
    bvh.add_triangle(Triangle(lw4, lw3, lw1, red_diffuse));

	// short box
    vec3 sb1 = vec3(130, 165, 65), sb2 = vec3(82, 165, 225), sb3 = vec3(240, 165, 272),
         sb4 = vec3(290, 165, 114);
    bvh.add_triangle(Triangle(sb1, sb2, sb3, white_diffuse));
    bvh.add_triangle(Triangle(sb4, sb3, sb1, white_diffuse));
    vec3 sb5 = vec3(290, 0, 114), sb6 = vec3(290, 165, 114), sb7 = vec3(240, 165, 272),
         sb8 = vec3(240, 0, 272);
    bvh.add_triangle(Triangle(sb5, sb6, sb7, white_diffuse));
    bvh.add_triangle(Triangle(sb8, sb7, sb5, white_diffuse));
    vec3 sb9 = vec3(130, 0, 65), sb10 = vec3(130, 165, 65), sb11 = vec3(290, 165, 114),
         sb12 = vec3(290, 0, 114);
    bvh.add_triangle(Triangle(sb9, sb10, sb11, white_diffuse));
    bvh.add_triangle(Triangle(sb12, sb11, sb9, white_diffuse));
    vec3 sb13 = vec3(82, 0, 225), sb14 = vec3(82, 165, 225), sb15 = vec3(130, 165, 65),
         sb16 = vec3(130, 0, 65);
    bvh.add_triangle(Triangle(sb13, sb14, sb15, white_diffuse));
    bvh.add_triangle(Triangle(sb16, sb15, sb13, white_diffuse));
    vec3 sb17 = vec3(240, 0, 272), sb18 = vec3(240, 165, 272), sb19 = vec3(82, 165, 225),
         sb20 = vec3(82, 0, 225);
    bvh.add_triangle(Triangle(sb17, sb18, sb19, white_diffuse));
    bvh.add_triangle(Triangle(sb20, sb19, sb17, white_diffuse));

	// tall box
    vec3 tb1 = vec3(423, 330, 247), tb2 = vec3(265, 330, 296), tb3 = vec3(314, 330, 456),
         tb4 = vec3(472, 330, 406);
    bvh.add_triangle(Triangle(tb1, tb2, tb3, white_diffuse));
    bvh.add_triangle(Triangle(tb1, tb3, tb4, white_diffuse));
    vec3 tb5 = vec3(423, 0, 247), tb6 = vec3(423, 330, 247), tb7 = vec3(472, 330, 406),
         tb8 = vec3(472, 0, 406);
    bvh.add_triangle(Triangle(tb5, tb6, tb7, white_diffuse));
    bvh.add_triangle(Triangle(tb5, tb7, tb8, white_diffuse));
    vec3 tb9 = vec3(472, 0, 406), tb10 = vec3(472, 330, 406), tb11 = vec3(314, 330, 456),
         tb12 = vec3(314, 0, 456);
    bvh.add_triangle(Triangle(tb9, tb10, tb11, white_diffuse));
    bvh.add_triangle(Triangle(tb9, tb11, tb12, white_diffuse));
    vec3 tb13 = vec3(314, 0, 456), tb14 = vec3(314, 330, 456), tb15 = vec3(265, 330, 296),
         tb16 = vec3(265, 0, 296);
    bvh.add_triangle(Triangle(tb13, tb14, tb15, white_diffuse));
    bvh.add_triangle(Triangle(tb13, tb15, tb16, white_diffuse));
    vec3 tb17 = vec3(265, 0, 296), tb18 = vec3(265, 330, 296), tb19 = vec3(423, 330, 247),
         tb20 = vec3(423, 0, 247);
    bvh.add_triangle(Triangle(tb17, tb18, tb19, white_diffuse));
    bvh.add_triangle(Triangle(tb17, tb19, tb20, white_diffuse));

    Camera camera = Camera(vec3(278, 278, -500), vec3(0, 0, 1), vec3(0, 1, 0), ivec2(1024, 1024),
                           60 * DEG2RAD, 1);
    render_gpu(camera, bvh, 10000, 5, ivec2(100, 100), argv[1]);
}