# Path Tracer

Path tracing algorithm implemented in C++ that supports the following:

- object-oriented design
- look from/at camera model
- triangles
- lambertian diffuse
- specular lighting
- emission and area lights
- global illumination
- soft shadows
- BVH acceleration
- CPU/GPU rendering
- OBJ file loading

## Examples

to do

## Building

This project uses [Bazel](https://bazel.build/install) for building. To build any of the examples:

```bash
bazel build //examples:example-name
```

And to run the executable:

```bash
./bazel-bin/examples/example-name
```

## To Do

- explicit light sampling [here](https://computergraphics.stackexchange.com/questions/5152/progressive-path-tracing-with-explicit-light-sampling/5153#5153?newreg=ba3a51d61bf64da5a1b3a589287511b2)
  - punctual (point) light sources
  - light attenuation
- skybox

- use OpenGL for GPU rendering instead of SFML (idk why shader only allows ~350 triangles)
