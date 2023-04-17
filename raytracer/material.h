#pragma once

#include "linalg.h"

enum MaterialType {
    DIFFUSE,
    SPECULAR,
    REFLECTIVE,
};

struct Material {
    MaterialType type;
    vec3 color;

    virtual ~Material() = default;
};

struct Diffuse : public Material {
    // lambertian diffuse
};

struct Specular : public Material {
    // specular reflection
};

struct Reflective : public Material {
    // mirror like reflection
};