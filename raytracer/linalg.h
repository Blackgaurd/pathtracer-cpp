#pragma once

#include <array>
#include <cmath>
#include <functional>
#include <iostream>

#ifndef NO_ROW_MAJOR
#define ROW_MAJOR
#endif

#define EPS 1e-6

struct vec2 {
    float x, y;

    vec2() = default;
    vec2(float v) : x(v), y(v) {}
    vec2(float x, float y) : x(x), y(y) {}
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
    vec3 apply(std::function<float(float)> f) const {
        return vec3(f(x), f(y), f(z));
    }

    friend std::ostream& operator<<(std::ostream& os, const vec3& v) {
        return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    }
};
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
        return vec3(
            v.dot({arr[0][0], arr[1][0], arr[2][0]}),
            v.dot({arr[0][1], arr[1][1], arr[2][1]}),
            v.dot({arr[0][2], arr[1][2], arr[2][2]}));
#else
        return vec3(
            v.dot({arr[0][0], arr[0][1], arr[0][2]}),
            v.dot({arr[1][0], arr[1][1], arr[1][2]}),
            v.dot({arr[2][0], arr[2][1], arr[2][2]}));
#endif
    }

    friend std::ostream& operator<<(std::ostream& os, const mat4& m) {
        for (size_t i = 0; i < 4; i++) {
            os << "[";
            for (size_t j = 0; j < 4; j++)
                os << m[i][j] << (j == 3 ? "" : ", ");
            os << "]\n";
        }
        return os;
    }
};
namespace mat4_constructors {
mat4 identity() {
    mat4 m(0);
    for (size_t i = 0; i < 4; i++)
        m[i][i] = 1;
    return m;
}
mat4 camera(const vec3& look_from, const vec3& look_at, const vec3& up) {
    vec3 forward = (look_from - look_at).normalize();

    if (forward.angle(up) < EPS || forward.angle(-up) < EPS) {
        std::cerr << "forward and up vectors are parallel" << std::endl;
        return identity();
    }

    vec3 left = up.cross(forward).normalize();
    vec3 true_up = forward.cross(left).normalize();

    mat4 m = identity();
#define set_row(i, v) \
    m[i][0] = v.x;    \
    m[i][1] = v.y;    \
    m[i][2] = v.z;
    set_row(0, left);
    set_row(1, true_up);
    set_row(2, forward);
    set_row(3, look_from);
#undef set_row

    return m;
}
}  // namespace mat4_constructors

float clamp(float x, float min, float max) {
    return std::max(min, std::min(max, x));
}
