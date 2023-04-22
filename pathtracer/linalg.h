#pragma once

// make sure c++17
static_assert(__cplusplus >= 201703L, "C++17 required");

#include <array>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>

#ifndef NO_ROW_MAJOR
#define ROW_MAJOR
#endif

#define EPS 1e-6

struct Resolution {
    int width, height;

    Resolution() = default;
    Resolution(int width, int height) : width(width), height(height) {}

    bool operator==(const Resolution& o) const {
        return width == o.width && height == o.height;
    }
    bool operator!=(const Resolution& o) const {
        return !(*this == o);
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
    vec3 operator-() const {
        return vec3(-x, -y, -z);
    }
    bool operator==(const vec3& o) const {
        return x == o.x && y == o.y && z == o.z;
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
#ifdef ROW_MAJOR
        return vec3(v.dot({arr[0][0], arr[1][0], arr[2][0]}),
                    v.dot({arr[0][1], arr[1][1], arr[2][1]}),
                    v.dot({arr[0][2], arr[1][2], arr[2][2]}));
#else
        return vec3(v.dot({arr[0][0], arr[0][1], arr[0][2]}),
                    v.dot({arr[1][0], arr[1][1], arr[1][2]}),
                    v.dot({arr[2][0], arr[2][1], arr[2][2]}));
#endif
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
