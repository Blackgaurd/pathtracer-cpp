#pragma once

#include "camera.h"
#include "image.h"
#include "light.h"
#include "linalg.h"
#include "object.h"
#include "rng.h"
#include "scene.h"

#define RAYTRACER_VERSION "0.1.0"

static_assert(__cplusplus >= 201703L, "raytracer requires C++17");
