#pragma once

#include <SFML/Graphics.hpp>
#include <cmath>

const float EPS = 1e-6;

#define vec3_op(op)                                                                \
    sf::Glsl::Vec3 operator op(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) { \
        return sf::Glsl::Vec3(a.x op b.x, a.y op b.y, a.z op b.z);                 \
    }                                                                              \
    sf::Glsl::Vec3 operator op(const sf::Glsl::Vec3& a, float b) {                 \
        return sf::Glsl::Vec3(a.x op b, a.y op b, a.z op b);                       \
    }                                                                              \
    sf::Glsl::Vec3 operator op(float a, const sf::Glsl::Vec3& b) {                 \
        return sf::Glsl::Vec3(a op b.x, a op b.y, a op b.z);                       \
    }                                                                              \
    sf::Glsl::Vec3& operator op##=(sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {   \
        a.x op## = b.x;                                                            \
        a.y op## = b.y;                                                            \
        a.z op## = b.z;                                                            \
        return a;                                                                  \
    }                                                                              \
    sf::Glsl::Vec3& operator op##=(sf::Glsl::Vec3& a, float b) {                   \
        a.x op## = b;                                                              \
        a.y op## = b;                                                              \
        a.z op## = b;                                                              \
        return a;                                                                  \
    }
vec3_op(+);
vec3_op(-);
vec3_op(*);
vec3_op(/);
#undef vec3_op

float get(const sf::Glsl::Vec3& a, int i) {
    if (i == 0) return a.x;
    if (i == 1) return a.y;
    if (i == 2) return a.z;
    throw std::runtime_error("Invalid index");
}
sf::Glsl::Vec3 operator-(const sf::Glsl::Vec3& a) {
    return sf::Glsl::Vec3(-a.x, -a.y, -a.z);
}
bool operator==(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {
    return std::abs(a.x - b.x) < EPS && std::abs(a.y - b.y) < EPS && std::abs(a.z - b.z) < EPS;
}
bool operator!=(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {
    return !(a == b);
}

float dot(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
sf::Glsl::Vec3 cross(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {
    return sf::Glsl::Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
float length(const sf::Glsl::Vec3& a) {
    return std::sqrt(dot(a, a));
}
sf::Glsl::Vec3 normalize(const sf::Glsl::Vec3& a) {
    return a / length(a);
}
float angle(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {
    return std::acos(dot(a, b) / (length(a) * length(b)));
}
float distance(const sf::Glsl::Vec3& a, const sf::Glsl::Vec3& b) {
    return length(a - b);
}
sf::Glsl::Vec3 pow(const sf::Glsl::Vec3& a, float b) {
    return sf::Glsl::Vec3(std::pow(a.x, b), std::pow(a.y, b), std::pow(a.z, b));
}