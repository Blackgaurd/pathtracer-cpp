#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "bvh.h"
#include "camera.h"
#include "fpng.h"
#include "linalg.h"

// #define DEBUG

// glsl sources are const char* and
// not std::string because of the
// way they are passed to OpenGL
const char* VERT_SOURCE = R"glsl(#version 330 core

attribute vec2 position;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}
)glsl";

const char* FRAG_SOURCE = R"glsl(#version 330 core

precision mediump float;

const float PI = 3.1415926538f;
const float PI_2 = 1.5707963268f;
const float FLOAT_INF = 1e30f;
const float BIAS = 1e-4f;
const float EPS = 1e-6f;

#define MAX_DEPTH 5

uniform int render_samples;
uniform int render_depth;

#ifdef REALTIME
uniform int frame;
uniform sampler2D prev_frame;
#endif

struct Camera {
    vec3 pos;
    ivec2 res;
    vec2 v_res;
    float cell_size;
    float image_distance;
    mat4 transform;
};
uniform Camera camera;

const int EMIT = 1;
const int DIFF = 2; // diffuse
const int SPEC = 3; // specular
struct Material {
    int type;
    vec3 color, emit_color;
    float roughness;
};
struct AABB {
    vec3 lb, rt;
};
struct Triangle {
    vec3 v1, v2, v3;
    Material material;
};
struct BVHNode {
    AABB aabb;
    int left, right;
    int tri_start, tri_end;
};

#define MAX_TRIANGLES 300
uniform Triangle triangles[MAX_TRIANGLES];
uniform int tri_indices[MAX_TRIANGLES];
uniform BVHNode bvh_nodes[MAX_TRIANGLES * 2];

float rand01(inout uint state) {
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    //state = (state * 196314165u) + 907633515u;
    return float(state) / 4294967295.0;
}

bool i_tri(vec3 ray_o, vec3 ray_d, int tri_idx, out float t) {
    vec3 v1 = triangles[tri_idx].v1;
    vec3 v2 = triangles[tri_idx].v2;
    vec3 v3 = triangles[tri_idx].v3;

    vec3 edge1 = v2 - v1, edge2 = v3 - v1;
    vec3 h = cross(ray_d, edge2);
    float a = dot(edge1, h);
    if (abs(a) < EPS)
        return false;

    float f = 1.0f / a;
    vec3 s = ray_o - v1;
    float u = f * dot(s, h);
    if (u < 0 || u > 1)
        return false;

    vec3 q = cross(s, edge1);
    float v = f * dot(ray_d, q);
    if (v < 0 || u + v > 1)
        return false;

    t = f * dot(edge2, q);
    return t > 0;
}
vec3 n_tri(vec3 ray_d, vec3 p, int tri_idx) {
    vec3 v1 = triangles[tri_idx].v1;
    vec3 v2 = triangles[tri_idx].v2;
    vec3 v3 = triangles[tri_idx].v3;

    vec3 edge1 = v2 - v1, edge2 = v3 - v1;
    vec3 n = normalize(cross(edge1, edge2));
    return dot(n, ray_d) < 0 ? n : -n;
}

float vec_min3(vec3 v) {
    return min(min(v.x, v.y), v.z);
}
float vec_max3(vec3 v) {
    return max(max(v.x, v.y), v.z);
}
vec3 component_min3(vec3 a, vec3 b) {
    return vec3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}
vec3 component_max3(vec3 a, vec3 b) {
    return vec3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}
bool i_aabb(vec3 ray_o, vec3 inv_ray_d, AABB abbb) {
    vec3 t1 = (abbb.lb - ray_o) * inv_ray_d;
    vec3 t2 = (abbb.rt - ray_o) * inv_ray_d;

    float tmax = vec_min3(component_max3(t1, t2));
    float tmin = vec_max3(component_min3(t1, t2));
    if (tmax < 0)
        return false;

    return tmin <= tmax;
}
bool is_leaf(int bvh_idx) {
    return bvh_nodes[bvh_idx].left == -1; // && bvh_nodes[bvh_idx].right == -1;
}
int i_bvh(vec3 ray_o, vec3 ray_d, out float t) {
    // returns the index of the triangle
    // that intersects the ray
    // or -1 if no intersection

    // stack based intersection algorithm
    // to mimic recursion

    int stack[MAX_TRIANGLES * 2];
    int stack_ptr = 0;
    stack[stack_ptr++] = 0;

    vec3 inv_ray_d = 1 / ray_d;
    int ret = -1;
    float min_t = FLOAT_INF;

    while (stack_ptr > 0) {
        stack_ptr--;
        int cur_idx = stack[stack_ptr];
        if (!i_aabb(ray_o, inv_ray_d, bvh_nodes[cur_idx].aabb))
            continue;
        if (is_leaf(cur_idx)) {
            int start = bvh_nodes[cur_idx].tri_start;
            int end = bvh_nodes[cur_idx].tri_end;
            for (int i = start; i <= end; i++) {
                float t_;
                if (i_tri(ray_o, ray_d, tri_indices[i], t_) && t_ < min_t) {
                    min_t = t_;
                    ret = tri_indices[i];
                }
            }
        } else {
            stack[stack_ptr++] = bvh_nodes[cur_idx].left;
            stack[stack_ptr++] = bvh_nodes[cur_idx].right;
        }
    }

    if (ret != -1)
        t = min_t;
    return ret;
}

vec3 reflect_d(vec3 ray_d, vec3 normal, int tri_id, inout uint seed) {
    // brdf for different materials
    int type = triangles[tri_id].material.type;

    if (type == SPEC) {
        // specular with noise
        vec3 reflected = reflect(ray_d, normal);
        float roughness = triangles[tri_id].material.roughness;
        vec3 ret;
        do {
            vec3 jitter = (vec3(rand01(seed), rand01(seed), rand01(seed)) - 0.5f) * roughness;
            ret = normalize(reflected + jitter);
        } while (dot(ret, normal) < 0);
        return ret;
    } else {
        // lambertian diffuse
        float u = rand01(seed), v = rand01(seed);
        float theta = acos(2 * u - 1) - PI_2;
        float phi = 2 * PI * v;
        vec3 sample = vec3(cos(theta) * cos(phi), cos(theta) * sin(phi), sin(theta));
        return sign(dot(sample, normal)) * sample;
    }
}

vec3 trace(vec3 ray_o, vec3 ray_d, int depth, inout uint seed) {
    // stack based iteration

    struct TraceResult {
        vec3 color, emit;
        float theta;
    } stack[MAX_DEPTH + 1];
    int stack_ptr = 0;

    for (int d = 0; d < depth; d++) {
        float hit_t = FLOAT_INF;
        int best_i = i_bvh(ray_o, ray_d, hit_t);

        if (best_i == -1)
            break;

        vec3 color = triangles[best_i].material.color;
        vec3 emit = triangles[best_i].material.emit_color;
        if (triangles[best_i].material.type == EMIT) {
            stack[stack_ptr++] = TraceResult(color, emit, 0);
            break;
        }

        vec3 hit_p = ray_o + ray_d * hit_t;
        vec3 hit_n = n_tri(ray_d, hit_p, best_i);
        vec3 bias = hit_n * BIAS;

        ray_o = hit_p + bias;
        ray_d = reflect_d(ray_d, hit_n, best_i, seed);
        float theta = dot(hit_n, ray_d);
        stack[stack_ptr++] = TraceResult(color, emit, theta);
    }

    vec3 color = vec3(0);
    for (stack_ptr--; stack_ptr >= 0; stack_ptr--) {
        // multiply by 2 to account for cosine weighted hemisphere
        color = stack[stack_ptr].emit + 2 * color * stack[stack_ptr].color * stack[stack_ptr].theta;
    }

    return color;
}

vec3 normal_shade(vec3 ray_o, vec3 ray_d) {
    float hit_t = FLOAT_INF;
    int best_i = i_bvh(ray_o, ray_d, hit_t);

    if (best_i == -1)
        return vec3(0);

    vec3 hit_p = ray_o + ray_d * hit_t;
    vec3 hit_n = n_tri(ray_d, hit_p, best_i);
    return hit_n;
}

vec3 camera_ray(inout uint seed) {
    float w = gl_FragCoord.x, h = gl_FragCoord.y;
    vec2 jitter = vec2(rand01(seed), rand01(seed)) * camera.cell_size;

    vec3 ray_d = vec3(w * camera.cell_size - camera.v_res.x / 2 + jitter.x, h * camera.cell_size - camera.v_res.y / 2 + jitter.y, -camera.image_distance);
    ray_d = vec3(dot(ray_d, vec3(camera.transform[0][0], camera.transform[1][0], camera.transform[2][0])), dot(ray_d, vec3(camera.transform[0][1], camera.transform[1][1], camera.transform[2][1])), dot(ray_d, vec3(camera.transform[0][2], camera.transform[1][2], camera.transform[2][2])));
    return normalize(ray_d);
}
void main() {
    // acts weird if seed = 0
    #ifdef REALTIME
    uint seed = uint(frame * gl_FragCoord.y + gl_FragCoord.x * camera.res.y + 1);
    #else
    uint seed = uint(gl_FragCoord.y + gl_FragCoord.x * camera.res.y + 1);
    #endif

    vec3 cur_color = vec3(0);
    for (int i = 0; i < render_samples; i++) {
        vec3 ray_d = camera_ray(seed);
        vec3 color = trace(camera.pos, ray_d, render_depth, seed);
        cur_color += color / render_samples;
    }

    // gamma correction
    cur_color = pow(cur_color, vec3(1.0 / 2.2));

    #ifdef REALTIME
    vec2 pos = gl_FragCoord.xy / camera.res.xy;
    vec3 prev_color = texture(prev_frame, pos).rgb;

    vec3 color = mix(prev_color, cur_color, 1 / float(frame + 1));
    gl_FragColor = vec4(color, 1.0);
    #else
    gl_FragColor = vec4(cur_color, 1.0);
    #endif
}
)glsl";

#ifdef DEBUG
const char* test_frag_source = R"glsl(
#version 330 core

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(256, 256) * 2 - 1;
    gl_FragColor = vec4(abs(uv), 0, 1);
}
)glsl";
#endif

struct PathtraceShader {
    // renders a scene to an image

    GLFWwindow* window;
    ivec2 resolution;
    GLuint texture;
    GLuint fbo;
    GLuint vert_shader, frag_shader, shader;
    GLuint vao, vbo, ebo;

    PathtraceShader(const Camera& camera, BVH& bvh, int samples, int depth) {
        bool success = init_gl(camera.res);
        if (!success) throw std::runtime_error("Failed to initialize PathtraceShader");

        if (!bvh.built) {
            std::cerr << "Warning: BVH not built\nBuilding BVH...\n";
            bvh.build();
        }

        set_camera(camera);
        set_bvh(bvh);
        set_uniform("render_samples", samples);
        set_uniform("render_depth", depth);
    }
    ~PathtraceShader() {
        glDeleteTextures(1, &texture);
        glDeleteFramebuffers(1, &fbo);
        glDeleteShader(vert_shader);
        glDeleteShader(frag_shader);
        glDeleteProgram(shader);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glfwTerminate();
    }

// uniform wrapper functions
#define loc(name) glGetUniformLocation(shader, name.c_str())
    void set_uniform(const std::string& name, int value) {
        glUniform1i(loc(name), value);
    }
    void set_uniform(const std::string& name, float value) {
        glUniform1f(loc(name), value);
    }
    void set_uniform(const std::string& name, const ivec2& value) {
        glUniform2i(loc(name), value.x, value.y);
    }
    void set_uniform(const std::string& name, const vec2& value) {
        glUniform2f(loc(name), value.x, value.y);
    }
    void set_uniform(const std::string& name, const vec3& value) {
        glUniform3f(loc(name), value.x, value.y, value.z);
    }
    void set_uniform(const std::string& name, const std::array<float, 16>& value) {
        glUniformMatrix4fv(loc(name), 1, GL_FALSE, value.data());
    }
#undef loc

    void set_camera(const Camera& camera) {
        set_uniform("camera.pos", camera.pos);
        set_uniform("camera.res", camera.res);
        set_uniform("camera.v_res", camera.v_res);
        set_uniform("camera.cell_size", camera.cell_size);
        set_uniform("camera.image_distance", camera.distance);
        set_uniform("camera.transform", camera.transform);
    }
    void set_bvh(const BVH& bvh) {
        // set triangle indices
        for (size_t i = 0; i < bvh.tri_idx.size(); i++) {
            std::string name = "tri_indices[" + std::to_string(i) + "]";
            set_uniform(name, bvh.tri_idx[i]);
        }

        // set triangles
        for (size_t i = 0; i < bvh.triangles.size(); i++) {
            const Triangle& tri = bvh.triangles[i];
            std::string name = "triangles[" + std::to_string(i) + "].";
            set_uniform(name + "v1", tri.v1);
            set_uniform(name + "v2", tri.v2);
            set_uniform(name + "v3", tri.v3);

            name += "material.";
            set_uniform(name + "type", tri.material.type);
            set_uniform(name + "color", tri.material.color);
            set_uniform(name + "emit_color", tri.material.emit_color);
            set_uniform(name + "roughness", tri.material.roughness);
        }

        // set bvh nodes
        for (size_t i = 0; i < bvh.nodes.size(); i++) {
            const BVHNode& node = bvh.nodes[i];
            std::string name = "bvh_nodes[" + std::to_string(i) + "].";
            set_uniform(name + "aabb.lb", node.aabb.lb);
            set_uniform(name + "aabb.rt", node.aabb.rt);
            set_uniform(name + "left", node.left);
            set_uniform(name + "right", node.right);
            set_uniform(name + "tri_start", node.tri_start);
            set_uniform(name + "tri_end", node.tri_end);
        }
    }

    bool init_gl(const ivec2& resolution) {
        this->resolution = resolution;

        // initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW\n";
            return false;
        }

        // create hidden window
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window =
            glfwCreateWindow(resolution.x, resolution.y, "pathtracer render target", NULL, NULL);
        if (!window) {
            std::cerr << "Failed to create GLFW window\n";
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(window);

        // initialize GLEW
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW\n";
            glfwTerminate();
            return false;
        }

        // create texture
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, resolution.x, resolution.y, 0, GL_RGB,
                     GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // create fbo
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Failed to create FBO\n";
            glfwTerminate();
            return false;
        }

        // compile shaders
        vert_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert_shader, 1, &VERT_SOURCE, NULL);
        glCompileShader(vert_shader);
        frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
#ifdef DEBUG
        glShaderSource(frag_shader, 1, &test_frag_source, NULL);
#else
        glShaderSource(frag_shader, 1, &FRAG_SOURCE, NULL);
#endif
        glCompileShader(frag_shader);
        shader = glCreateProgram();
        glAttachShader(shader, vert_shader);
        glAttachShader(shader, frag_shader);
        glLinkProgram(shader);
        glUseProgram(shader);

        return true;
    }
    void draw_rect(const ivec2& top_left, const ivec2& bottom_right) {
        // clamp top_left and bottom_right to resolution
        ivec2 c_tl = component_max(top_left, ivec2(0));
        ivec2 c_br = component_min(bottom_right, resolution);

        // create vertices from top left and bottom right
        vec2 tl = vec2(c_tl) / vec2(resolution) * 2 - 1;
        vec2 tr = vec2(c_br.x, c_tl.y) / vec2(resolution) * 2 - 1;
        vec2 bl = vec2(c_tl.x, c_br.y) / vec2(resolution) * 2 - 1;
        vec2 br = vec2(c_br) / vec2(resolution) * 2 - 1;
        std::array<float, 20> vertices = {
            tl.x, tl.y, 0, 0, 0, tr.x, tr.y, 0, 0, 0, bl.x, bl.y, 0, 0, 0, br.x, br.y, 0, 0, 0,
        };
        const std::array<GLuint, 6> indices = {
            0, 1, 3,  // first triangle
            0, 2, 3   // second triangle
        };

        // create vao, vbo, ebo
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(),
                     GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(),
                     GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        // render to fbo
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, resolution.x, resolution.y);  // ! maybe this can be moved to init_gl
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    void clear_buffer(const vec3& color) {
        // clear fbo to color
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(color.x, color.y, color.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    void finish() {
        // submit tasks to gpu
        glFinish();
    }

    void save_ppm(const std::string& filename) {
        glBindTexture(GL_TEXTURE_2D, texture);
        std::vector<GLubyte> pixels(resolution.x * resolution.y * 3);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        std::ofstream file(filename);
        file << "P3\n" << resolution.x << " " << resolution.y << "\n255\n";
        for (int i = 0; i < resolution.x * resolution.y * 3; i += 3) {
            file << (int)pixels[i] << " ";
            file << (int)pixels[i + 1] << " ";
            file << (int)pixels[i + 2] << "\n";
        }
    }
    void save_png(const std::string& filename) {
        glBindTexture(GL_TEXTURE_2D, texture);
        std::vector<GLubyte> pixels(resolution.x * resolution.y * 3);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

        // flip the image vertically
        for (int y = 0; y < resolution.y / 2; y++) {
            for (int x = 0; x < resolution.x; x++) {
                for (int c = 0; c < 3; c++) {
                    std::swap(pixels[(y * resolution.x + x) * 3 + c],
                              pixels[((resolution.y - y - 1) * resolution.x + x) * 3 + c]);
                }
            }
        }

        bool success = fpng::fpng_encode_image_to_file(filename.c_str(), pixels.data(),
                                                       resolution.x, resolution.y, 3);
        if (!success) std::cerr << "Failed to write image to file: " << filename << '\n';
    }
};