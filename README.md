# Path Tracer

Path tracing algorithm implemented in C++ that supports the following:

- extendable object-oriented design
- look from/at camera model
- spheres
- triangles
- lambertian diffuse
- specular lighting
- emission and area lights
- global illumination
- soft shadows
- BVH acceleration
- CPU multithreaded rendering
- OBJ file loading

## Examples

Two cats in the Cornell box (~4200 triangles) rendered at 1000 spp:

![cats-example](examples/cats/cats.png)

## Building

This project uses [Bazel](https://bazel.build/install) for building. To build any of the examples:

```bash
bazel build //examples:example-name
```

And to run the executable:

```bash
./bazel-bin/examples/example-name
```
