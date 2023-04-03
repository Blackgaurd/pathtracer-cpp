#pragma once

#include <cassert>
#include <fstream>
#include <vector>

#include "fpng/fpng.h"
#include "linalg.h"

struct image_t {
    int width, height;
    std::vector<std::vector<vec3>> pixels;

    image_t() = default;
    image_t(int width, int height) : width(width), height(height), pixels(height, std::vector<vec3>(width)) {}

    void set_pixel(int w, int h, const vec3& color) {
        pixels[h][w] = color;
    }
    vec3 get_pixel(int w, int h) const {
        assert(w >= 0 && w < width && h >= 0 && h < height && "pixel out of bounds");
        return pixels[h][w];
    }
    void write_png(std::string filename) {
        std::vector<unsigned char> data(width * height * 3);
        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++) {
                int index = (h * width + w) * 3;
                data[index + 0] = static_cast<unsigned char>(clamp(pixels[height - h - 1][w].x, 0, 1) * 255);
                data[index + 1] = static_cast<unsigned char>(clamp(pixels[height - h - 1][w].y, 0, 1) * 255);
                data[index + 2] = static_cast<unsigned char>(clamp(pixels[height - h - 1][w].z, 0, 1) * 255);
            }
        }

        bool success = fpng::fpng_encode_image_to_file(filename.c_str(), data.data(), width, height, 3);
        if (!success)
            std::cerr << "Failed to write image to file: " << filename << std::endl;
    }
    void write_ppm(std::string filename) {
        std::ofstream out(filename);
        out << "P6\n"
            << width << " " << height << "\n255\n";
        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++) {
                out << static_cast<unsigned char>(clamp(pixels[height - h - 1][w].x, 0, 1) * 255);
                out << static_cast<unsigned char>(clamp(pixels[height - h - 1][w].y, 0, 1) * 255);
                out << static_cast<unsigned char>(clamp(pixels[height - h - 1][w].z, 0, 1) * 255);
            }
        }
        out.close();
    }
};