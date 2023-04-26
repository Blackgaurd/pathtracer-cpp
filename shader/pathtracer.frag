#version 330

uniform int frame;
uniform ivec2 resolution;

uniform int frame_count;
uniform sampler2D prev_frame;

uniform vec3 look_from;
uniform vec2 v_res;
uniform float image_distance;
uniform mat4 camera;

const int EMIT = 1;
const int DIFF = 2; // diffuse
const int SPEC = 3; // specular
struct Material {
    int type;
    vec3 color;
    vec3 emit_color;
    float roughness;
};
struct Sphere {
    vec3 center;
    float radius;
    Material material;
};
struct Triangle {
    vec3 v1, v2, v3;
    Material material;
};

#define MAX_SPHERES 10
uniform int sphere_count;
uniform Sphere spheres[MAX_SPHERES];

#define MAX_TRIANGLES 5
uniform int triangle_count;
uniform Triangle triangles[MAX_TRIANGLES];

const float PI = 3.1415926538f;
const float PI_2 = 1.5707963268f;
const float FLOAT_INF = 1e30f;
const float BIAS = 1e-4f;
const float EPS = 1e-6f;

const int MAX_DEPTH = 5;

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

bool i_sphere(vec3 ray_o, vec3 ray_d, int sphere_idx, out float t) {
    vec3 center = spheres[sphere_idx].center;
    float radius = spheres[sphere_idx].radius;
    float b = 2.0f * dot(ray_d, ray_o - center);
    float c = dot(ray_o, ray_o) + dot(center, center) - 2.0f * dot(ray_o, center) - radius * radius;

    float d = b * b - 4.0f * c;
    if (d < 0)
        return false;

    t = (-b - sqrt(d)) / 2.0f;
    return t > 0;
}
vec3 n_sphere(vec3 ray_d, vec3 p, int sphere_idx) {
    vec3 center = spheres[sphere_idx].center;
    vec3 n = normalize(p - center);
    return dot(n, ray_d) < 0 ? n : -n;
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
    return normalize(cross(edge1, edge2));
}

vec3 reflect_d(vec3 ray_d, vec3 normal, int sphere_idx, inout uint seed) {
    // brdf for different materials
    int type = spheres[sphere_idx].material.type;

    // perfect hemispherical diffuse
    // not sure how to implement gaussian distribution
    // todo: make this the default
    if (type == DIFF) {
        float u = rand01(seed), v = rand01(seed);
        float theta = acos(2 * u - 1) - PI_2;
        float phi = 2 * PI * v;
        vec3 sample = vec3(cos(theta) * cos(phi), cos(theta) * sin(phi), sin(theta));
        return sign(dot(sample, normal)) * sample;
    }

    // specular with noise
    if (type == SPEC) {
        vec3 reflected = reflect(ray_d, normal);
        float roughness = spheres[sphere_idx].material.roughness;
        vec3 ret;
        do {
            vec3 jitter = (vec3(rand01(seed), rand01(seed), rand01(seed)) - 0.5f) * roughness;
            ret = normalize(reflected + jitter);
        } while (dot(ret, normal) < 0);
        return ret;
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
        int best_i = -1;
        for (int i = 0; i < sphere_count; i++) {
            float t;
            if (i_sphere(ray_o, ray_d, i, t) && t < hit_t) {
                hit_t = t;
                best_i = i;
            }
        }

        if (best_i == -1)
            break;

        vec3 color = spheres[best_i].material.color;
        vec3 emit = spheres[best_i].material.emit_color;
        if (spheres[best_i].material.type == EMIT) {
            stack[stack_ptr++] = TraceResult(color, emit, 0);
            break;
        }

        vec3 hit_p = ray_o + ray_d * hit_t;
        vec3 hit_n = n_sphere(ray_d, hit_p, best_i);
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
    int best_i = -1;
    for (int i = 0; i < sphere_count; i++) {
        float t;
        if (i_sphere(ray_o, ray_d, i, t) && t < hit_t) {
            hit_t = t;
            best_i = i;
        }
    }

    if (best_i == -1)
        return vec3(0);

    vec3 hit_p = ray_o + ray_d * hit_t;
    vec3 hit_n = n_sphere(ray_d, hit_p, best_i);
    return hit_n;
}

void camera_ray(out vec3 ray_o, out vec3 ray_d, inout uint seed) {
    float w = gl_FragCoord.x, h = gl_FragCoord.y;
    float cell_size = v_res.x / resolution.x;
    vec2 jitter = vec2(rand01(seed), rand01(seed)) * cell_size;

    ray_d = vec3(w * cell_size - v_res.x / 2 + jitter.x, h * cell_size - v_res.y / 2 + jitter.y, -image_distance);
    ray_d = vec3(dot(ray_d, vec3(camera[0][0], camera[1][0], camera[2][0])), dot(ray_d, vec3(camera[0][1], camera[1][1], camera[2][1])), dot(ray_d, vec3(camera[0][2], camera[1][2], camera[2][2])));
    ray_d = normalize(ray_d);
    ray_o = look_from;
}
void main() {
    // acts weird if seed = 0
    uint seed = uint(frame * gl_FragCoord.y + gl_FragCoord.x * resolution.y);

    const int samples = 30;
    vec3 cur_color = vec3(0);
    for (int i = 0; i < samples; i++) {
        vec3 ray_o, ray_d;
        camera_ray(ray_o, ray_d, seed);
        vec3 color = trace(ray_o, ray_d, 3, seed);
        cur_color += color / samples;
    }
    cur_color = pow(cur_color, vec3(1.0 / 2.2));

    if (frame_count == 0) {
        gl_FragColor = vec4(cur_color, 1.0);
        return;
    }

    vec2 pos = gl_FragCoord.xy / resolution.xy;
    // vec3 prev_color = texture2D(prev_frame, pos).rgb;
    vec3 prev_color = texture(prev_frame, pos).rgb;
    gl_FragColor = vec4(prev_color, 1.0);

    vec3 color = mix(prev_color, cur_color, 1 / float(frame_count + 1));
    gl_FragColor = vec4(color, 1.0);
}