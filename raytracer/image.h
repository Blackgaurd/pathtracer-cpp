#pragma once

#include <fstream>
#include <stdexcept>
#include <vector>

#include "fpng/fpng.h"
#include "linalg.h"

struct Image {
    Resolution res;
    std::vector<std::vector<vec3>> pixels;

    Image() = default;
    Image(const Resolution& resolution)
        : res(resolution),
          pixels(resolution.height, std::vector<vec3>(resolution.width)) {}

    void set_pixel(int w, int h, const vec3& color) {
        if (w < 0 || w >= res.width || h < 0 || h >= res.height)
            throw std::out_of_range("pixel out of range");
        pixels[h][w] = color;
    }
    vec3 get_pixel(int w, int h) const {
        if (w < 0 || w >= res.width || h < 0 || h >= res.height)
            throw std::out_of_range("pixel out of range");
        return pixels[h][w];
    }
    void add_layer(const Image& layer) {
        if (res != layer.res)
            throw std::invalid_argument("resolution mismatch");
        for (int h = 0; h < res.height; h++)
            for (int w = 0; w < res.width; w++)
                pixels[h][w] += layer.pixels[h][w];
    }
    void divide(float factor) {
        for (int h = 0; h < res.height; h++)
            for (int w = 0; w < res.width; w++)
                pixels[h][w] /= factor;
    }
    void write_png(std::string filename) {
        std::vector<unsigned char> data(res.width * res.height * 3);
        for (int h = 0; h < res.height; h++) {
            for (int w = 0; w < res.width; w++) {
                int index = (h * res.width + w) * 3;
                data[index + 0] = static_cast<unsigned char>(
                    clamp(pixels[res.height - h - 1][w].x, 0, 1) * 255);
                data[index + 1] = static_cast<unsigned char>(
                    clamp(pixels[res.height - h - 1][w].y, 0, 1) * 255);
                data[index + 2] = static_cast<unsigned char>(
                    clamp(pixels[res.height - h - 1][w].z, 0, 1) * 255);
            }
        }

        bool success = fpng::fpng_encode_image_to_file(
            filename.c_str(), data.data(), res.width, res.height, 3);
        if (!success)
            std::cerr << "Failed to write image to file: " << filename
                      << std::endl;
    }
    void write_ppm(std::string filename) {
        std::ofstream out(filename);
        out << "P6\n" << res.width << " " << res.height << "\n255\n";
        for (int h = 0; h < res.height; h++) {
            for (int w = 0; w < res.width; w++) {
                out << static_cast<unsigned char>(
                    clamp(pixels[res.height - h - 1][w].x, 0, 1) * 255);
                out << static_cast<unsigned char>(
                    clamp(pixels[res.height - h - 1][w].y, 0, 1) * 255);
                out << static_cast<unsigned char>(
                    clamp(pixels[res.height - h - 1][w].z, 0, 1) * 255);
            }
        }
        out.close();
    }
};