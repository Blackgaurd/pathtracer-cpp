#pragma once

#include <iomanip>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include "bvh.h"
#include "camera.h"
#include "image.h"
#include "linalg.h"
#include "material.h"
#include "object.h"
#include "tiny_obj_loader.h"

struct Scene {
    std::vector<ObjectPtr> objects, bvh_objects;
    BVHNodePtr bvh_root;

    float shift_bias = 1e-4;

    Scene() = default;

    template <typename T, typename... Args>
    void bvh_add_object(Args&&... args) {
        // add an object to be accelerated by the BVH
        bvh_objects.push_back(std::make_shared<T>(std::forward<Args>(args)...));
    }
    template <typename T, typename... Args>
    void add_object(Args&&... args) {
        // add an object to the scene (not accelerated by the BVH)
        objects.push_back(std::make_shared<T>(std::forward<Args>(args)...));
    }
    void load_obj(const std::string& filename, const std::string& mtl_path = "./") {
        tinyobj::ObjReaderConfig reader_config;
        reader_config.mtl_search_path = mtl_path;

        tinyobj::ObjReader reader;
        if (!reader.ParseFromFile(filename, reader_config)) {
            if (!reader.Error().empty())
                throw std::runtime_error("TinyObjLoader: " + reader.Error());
            throw std::runtime_error("TinyObjLoader: unknown error");
        }

        if (!reader.Warning().empty()) {
            std::cout << "TinyObjLoader: " << reader.Warning() << std::endl;
        }

        const auto& attrib = reader.GetAttrib();
        const auto& shapes = reader.GetShapes();
        const auto& materials = reader.GetMaterials();

        for (const auto& shape : shapes) {
            size_t index_offset = 0;
            for (size_t poly = 0; poly < shape.mesh.num_face_vertices.size(); poly++) {
                size_t vertices = shape.mesh.num_face_vertices[poly];
                std::vector<vec3> points;
                for (size_t v = 0; v < vertices; v++) {
                    auto idx = shape.mesh.indices[index_offset + v];
                    auto vx = attrib.vertices[3 * idx.vertex_index];
                    auto vy = attrib.vertices[3 * idx.vertex_index + 1];
                    auto vz = attrib.vertices[3 * idx.vertex_index + 2];
                    points.emplace_back(vx, vy, vz);
                }
                index_offset += vertices;

                int mat_id = shape.mesh.material_ids[poly];
                auto& mat = materials[mat_id];
                MaterialPtr material;
                switch (mat.illum) {
                    case 1: {
                        // diffuse
                        vec3 color = vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
                        material = std::make_shared<Diffuse>(color);
                        break;
                    }
                    case 2: {
                        // emit
                        vec3 color = vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
                        material = std::make_shared<Emit>(color);
                        break;
                    }
                    default: {
                        std::cerr << "Unknown material type with illum: " << mat.illum << std::endl;
                        std::cerr << "Using default material: Diffuse(0.5)" << std::endl;
                        material = std::make_shared<Diffuse>(vec3(0.5));
                    }
                }

                bvh_add_object<Triangle>(points[0], points[1], points[2], material);
            }
        }
    }

    void render(const Camera& camera, Image& image, int depth, int samples) {
        if (objects.empty()) throw std::runtime_error("no objects in scene");

        std::cout << "Building BVH..." << std::endl;
        bvh_root = build_bvh(bvh_objects);
        std::cout << "Done" << std::endl;
        // return;

        std::cout << std::fixed << std::setprecision(2);
        Resolution res = image.res;
        for (int h = 0; h < res.height; h++) {
            float height_p = (float)h / res.height * 100;
            std::cout << "\rRendering: " << height_p << "%" << std::flush;
            for (int w = 0; w < res.width; w++) {
                vec3 ray_o, ray_d;
                vec3 color = {0, 0, 0};
                for (int s = 0; s < samples; s++) {
                    camera.get_ray(w, h, ray_o, ray_d);
                    color += path_trace_iter(ray_o, ray_d, depth);
                }
                image.set_pixel(w, h, color / samples);
            }
        }
    }
    void run_thread(const Camera& camera, Image& image, int depth, int samples) {
        Resolution res = image.res;
        vec3 ray_o, ray_d;
        for (int h = 0; h < res.height; h++) {
            for (int w = 0; w < res.width; w++) {
                vec3 color = {0, 0, 0};
                for (int s = 0; s < samples; s++) {
                    camera.get_ray(w, h, ray_o, ray_d);
                    color += path_trace_iter(ray_o, ray_d, depth);
                }
                vec3 prev_color = image.get_pixel(w, h);
                image.set_pixel(w, h, prev_color + color);
            }
        }
    }
    void render_threaded(const Camera& camera, Image& image, int depth, int samples, int threads) {
        // render on multiple threads
        if (objects.empty()) throw std::runtime_error("no objects in scene");

        int max_threads = std::thread::hardware_concurrency();
        if (threads > max_threads - 1) {
            throw std::runtime_error("more than " + std::to_string(max_threads - 1) +
                                     " threads not supported");
        }

        std::cout << "Building BVH..." << std::endl;
        bvh_root = build_bvh(bvh_objects);
        std::cout << "Done" << std::endl;

        std::vector<std::thread> pool(threads);
        std::vector<Image> images(threads, Image(image.res));

        for (int i = 0; i < threads; i++) {
            int sub_samples = samples / threads + (i < samples % threads);
            pool[i] = std::thread(&Scene::run_thread, this, std::ref(camera), std::ref(images[i]),
                                  depth, sub_samples);
        }
        for (int i = 0; i < threads; i++) pool[i].join();
        for (const Image& img : images) image += img;
        image /= samples;
    }
    ObjectPtr intersect(const vec3& ray_o, const vec3& ray_d, float& hit_t,
                        const ObjectPtr& ignore = nullptr) {
        float bvh_t;
        ObjectPtr bvh_hit_obj = intersect_bvh(bvh_root, ray_o, ray_d, hit_t, ignore);

        float bf_t;  // brute force
        ObjectPtr hit_obj = nullptr;
        hit_t = 1e30;
        for (ObjectPtr& obj : objects) {
            if (obj == ignore) continue;

            float t;
            if (obj->intersect(ray_o, ray_d, t) && t < bf_t) {
                bf_t = t;
                hit_obj = obj;
            }
        }

        if (bvh_hit_obj && hit_obj) {
            if (bvh_t < bf_t) {
                hit_t = bvh_t;
                return bvh_hit_obj;
            } else {
                hit_t = bf_t;
                return hit_obj;
            }
        } else if (bvh_hit_obj) {
            hit_t = bvh_t;
            return bvh_hit_obj;
        } else if (hit_obj) {
            hit_t = bf_t;
            return hit_obj;
        }
        return nullptr;
    }
    vec3 path_trace(const vec3& ray_o, const vec3& ray_d, int depth) {
        // recursive path tracing
        if (depth == 0) return {0, 0, 0};

        float hit_t;
        ObjectPtr hit_obj = intersect(ray_o, ray_d, hit_t);
        if (!hit_obj) return {0, 0, 0};

        if (hit_obj->material->type == EMIT) {
            return hit_obj->material->emission;
        }

        vec3 hit_p = ray_o + ray_d * hit_t;
        vec3 hit_n = hit_obj->normal(ray_d, hit_p);
        vec3 bias = hit_n * shift_bias;

        vec3 new_d = hit_obj->material->reflected_dir(ray_d, hit_n);
        vec3 new_o = hit_p + bias;

        vec3 rec_color = path_trace(new_o, new_d, depth - 1);
        vec3 emission = hit_obj->material->emission;
        vec3 surface_color = hit_obj->material->color;
        float theta = hit_n.dot(new_d);

        // idk where the 2 comes from
        return emission + 2 * rec_color * surface_color * theta;
    }
    vec3 path_trace_iter(const vec3& ray_o, const vec3& ray_d, int depth) {
        // iterative path tracing
        vec3 color = {0, 0, 0};
        vec3 ray_origin = ray_o;
        vec3 ray_direction = ray_d;
        for (int i = 0; i < depth; i++) {
            float hit_t;
            ObjectPtr hit_obj = intersect_bvh(bvh_root, ray_origin, ray_direction, hit_t);
            if (!hit_obj) break;

            if (hit_obj->material->type == EMIT) {
                color += hit_obj->material->emission;
                break;
            }

            vec3 hit_p = ray_origin + ray_direction * hit_t;
            vec3 hit_n = hit_obj->normal(ray_direction, hit_p);
            vec3 bias = hit_n * shift_bias;

            vec3 new_d = hit_obj->material->reflected_dir(ray_direction, hit_n);
            vec3 new_o = hit_p + bias;

            vec3 emission = hit_obj->material->emission;
            vec3 surface_color = hit_obj->material->color;
            float theta = hit_n.dot(new_d);

            // idk where the 2 comes from
            color += emission + 2 * surface_color * theta;

            ray_origin = new_o;
            ray_direction = new_d;
        }
        return color;
    }
};