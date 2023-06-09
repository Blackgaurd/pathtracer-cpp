#pragma once

// make sure c++17
static_assert(__cplusplus >= 201703L, "C++17 required");

#include <algorithm>
#include <cmath>
#include <ostream>

#define DEG2RAD M_PI / 180
#define EPS 1e-6
#define FLOAT_INF 1e30

struct ivec2 {
    int x, y;

    ivec2() = default;
    ivec2(int v) : x(v), y(v) {}
    ivec2(int x, int y) : x(x), y(y) {}

    bool operator==(const ivec2& other) {
        return x == other.x && y == other.y;
    }
    bool operator!=(const ivec2& other) {
        return !(*this == other);
    }

    friend ivec2 component_max(const ivec2& a, const ivec2& b) {
        return ivec2(std::max(a.x, b.x), std::max(a.y, b.y));
    }
    friend ivec2 component_min(const ivec2& a, const ivec2& b) {
        return ivec2(std::min(a.x, b.x), std::min(a.y, b.y));
    }
};

struct vec2 {
    float x, y;

    vec2() = default;
    vec2(float v) : x(v), y(v) {}
    vec2(float x, float y) : x(x), y(y) {}
    vec2(ivec2 v) : x(v.x), y(v.y) {}

#define vec2_op(op)                                   \
    vec2 operator op(const vec2& o) const {           \
        return vec2(x op o.x, y op o.y);              \
    }                                                 \
    vec2& operator op##=(const vec2& o) {             \
        x op## = o.x;                                 \
        y op## = o.y;                                 \
        return *this;                                 \
    }                                                 \
    vec2 operator op(float o) const {                 \
        return vec2(x op o, y op o);                  \
    }                                                 \
    vec2& operator op##=(float o) {                   \
        x op## = o;                                   \
        y op## = o;                                   \
        return *this;                                 \
    }                                                 \
    friend vec2 operator op(float o, const vec2& v) { \
        return vec2(o op v.x, o op v.y);              \
    }
    vec2_op(+);
    vec2_op(-);
    vec2_op(*);
    vec2_op(/);
#undef vec2_op

    bool operator==(const vec2& other) {
        return std::abs(x - other.x) < EPS && std::abs(y - other.y) < EPS;
    }
    bool operator!=(const vec2& other) {
        return !(*this == other);
    }

    friend std::ostream& operator<<(std::ostream& os, const vec2& v) {
        return os << "(" << v.x << ", " << v.y << ")";
    }
};

struct vec3 {
    float x, y, z;

    vec3() = default;
    vec3(float v) : x(v), y(v), z(v) {}
    vec3(float x, float y, float z) : x(x), y(y), z(z) {}

#define vec3_op(op)                                   \
    vec3 operator op(const vec3& o) const {           \
        return vec3(x op o.x, y op o.y, z op o.z);    \
    }                                                 \
    vec3& operator op##=(const vec3& o) {             \
        x op## = o.x;                                 \
        y op## = o.y;                                 \
        z op## = o.z;                                 \
        return *this;                                 \
    }                                                 \
    vec3 operator op(float o) const {                 \
        return vec3(x op o, y op o, z op o);          \
    }                                                 \
    vec3& operator op##=(float o) {                   \
        x op## = o;                                   \
        y op## = o;                                   \
        z op## = o;                                   \
        return *this;                                 \
    }                                                 \
    friend vec3 operator op(float o, const vec3& v) { \
        return vec3(o op v.x, o op v.y, o op v.z);    \
    }

    vec3_op(+);
    vec3_op(-);
    vec3_op(*);
    vec3_op(/);
#undef vec3_op

    float& operator[](int i) {
        if (i == 0) return x;
        if (i == 1) return y;
        if (i == 2) return z;
        throw std::out_of_range("vec3 index out of range");
    }
    float operator[](int i) const {
        if (i == 0) return x;
        if (i == 1) return y;
        if (i == 2) return z;
        throw std::out_of_range("vec3 index out of range");
    }
    vec3 operator-() const {
        return vec3(-x, -y, -z);
    }
    bool operator==(const vec3& o) const {
        return std::abs(x - o.x) < EPS && std::abs(y - o.y) < EPS && std::abs(z - o.z) < EPS;
    }
    bool operator!=(const vec3& o) const {
        return !(*this == o);
    }

    float dot(const vec3& o) const {
        return x * o.x + y * o.y + z * o.z;
    }
    vec3 cross(const vec3& o) const {
        return vec3(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x);
    }
    float length() const {
        return std::sqrt(dot(*this));
    }
    vec3 normalize() const {
        return *this / length();
    }
    float angle(const vec3& o) const {
        return std::acos(dot(o) / (length() * o.length()));
    }
    float distance(const vec3& o) const {
        return (*this - o).length();
    }
    vec3 reflect(const vec3& normal) const {
        return *this - normal * 2 * dot(normal);
    }
    float max() const {
        return std::max({x, y, z});
    }
    float min() const {
        return std::min({x, y, z});
    }
    vec3 apply(std::function<float(float)> f) const {
        return vec3(f(x), f(y), f(z));
    }

    friend vec3 component_max(const vec3& a, const vec3& b) {
        return vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
    }
    friend vec3 component_min(const vec3& a, const vec3& b) {
        return vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
    }
    friend vec3 pow(const vec3& v, float p) {
        return vec3(std::pow(v.x, p), std::pow(v.y, p), std::pow(v.z, p));
    }

    friend std::ostream& operator<<(std::ostream& os, const vec3& v) {
        return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    }
};
namespace color {
#define color_def(name, r, g, b) const vec3 name(r, g, b);
color_def(white, 1, 1, 1);
color_def(black, 0, 0, 0);
color_def(red, 1, 0, 0);
color_def(orange, 1, 0.5, 0);
color_def(yellow, 1, 1, 0);
color_def(green, 0, 1, 0);
color_def(blue, 0, 0, 1);
color_def(purple, 0.5, 0, 0.5);
#undef color_def

vec3 mix(const vec3& a, const vec3& b, float a_t = 0.5) {
    return a * (1 - a_t) + b * a_t;
}
}  // namespace color
struct mat4 {
    std::array<std::array<float, 4>, 4> arr;

    mat4() = default;
    mat4(const mat4&) = default;
    mat4(float v) {
        std::fill(arr.begin(), arr.end(), std::array<float, 4>{v, v, v, v});
    }

    std::array<float, 4>& operator[](size_t i) {
        return arr[i];
    }
    const std::array<float, 4>& operator[](size_t i) const {
        return arr[i];
    }

    vec3 transform_dir(const vec3& v) const {
        return vec3(v.dot({arr[0][0], arr[1][0], arr[2][0]}),
                    v.dot({arr[0][1], arr[1][1], arr[2][1]}),
                    v.dot({arr[0][2], arr[1][2], arr[2][2]}));
    }

    friend std::ostream& operator<<(std::ostream& os, const mat4& m) {
        for (size_t i = 0; i < 4; i++) {
            os << "[";
            for (size_t j = 0; j < 4; j++) os << m[i][j] << (j == 3 ? "" : ", ");
            os << "]\n";
        }
        return os;
    }
};

float clamp(float x, float min, float max) {
    return std::max(min, std::min(max, x));
}
