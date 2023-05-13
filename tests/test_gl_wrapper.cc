#include <iostream>

#include "pathtracer/pathtracer.h"

const std::string fragment_source = R"glsl(
#version 330 core
uniform vec2 resolution;
void main() {
    vec2 uv = gl_FragCoord.xy / resolution * 2.0 - 1.0;
    gl_FragColor = vec4(abs(uv), 0.0, 1.0);
}
)glsl";

int main() {
    RenderTarget target(64, 64);
    Shader shader;
    shader.init_string(fragment_source);
    shader.set_uniform("resolution", ivec2(64, 64));

    Rect rect(ivec2(0, 0), 32, 32);
    target.draw(rect, shader);
    target.finish();
    target.save_ppm("test.ppm");
}