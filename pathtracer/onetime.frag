#version 330

const float PI = 3.1415926538f;
const float PI_2 = 1.5707963268f;
const float FLOAT_INF = 1e30f;
const float BIAS = 1e-4f;
const float EPS = 1e-6f;

#define MAX_DEPTH 5

uniform int uniform_seed;
uniform int render_depth;
uniform int render_samples;

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
    AABB aabb;
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
        float roughness = triangles[tri_id].material.roughness;
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
    uint seed = uint(uniform_seed * gl_FragCoord.y + gl_FragCoord.x * camera.res.y + 1);

    vec3 color = vec3(0);
    for (int i = 0; i < render_samples; i++) {
        vec3 ray_d = camera_ray(seed);
        color += trace(camera.pos, ray_d, render_depth, seed);
    }
    color /= render_samples;
    // gamma correction
    color = pow(color, vec3(1 / 2.2));
    gl_FragColor = vec4(color, 1);
}