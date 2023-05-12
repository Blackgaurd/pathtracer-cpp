#pragma once

#include "fstream"
#include <iostream>

#include "fpng.h"
#include "linalg.h"

struct Image {
    ivec2 res;
    std::vector<std::vector<vec3>> pixels;

    Image() = default;
    Image(const ivec2& resolution)
        : res(resolution), pixels(resolution.y, std::vector<vec3>(resolution.x)) {}

    void set_pixel(int w, int h, const vec3& color) {
        if (w < 0 || w >= res.x || h < 0 || h >= res.y)
            throw std::out_of_range("pixel out of range");
        pixels[h][w] = color;
    }
    vec3 get_pixel(int w, int h) const {
        if (w < 0 || w >= res.x || h < 0 || h >= res.y)
            throw std::out_of_range("pixel out of range");
        return pixels[h][w];
    }
    vec3& get_pixel(int w, int h) {
        if (w < 0 || w >= res.x || h < 0 || h >= res.y)
            throw std::out_of_range("pixel out of range");
        return pixels[h][w];
    }
    void operator+=(const Image& other) {
        if (res != other.res) throw std::invalid_argument("image resolution mismatch");
        for (int h = 0; h < res.y; h++)
            for (int w = 0; w < res.x; w++) pixels[h][w] += other.pixels[h][w];
    }
    void operator/=(float scalar) {
        for (int h = 0; h < res.y; h++)
            for (int w = 0; w < res.x; w++) pixels[h][w] /= scalar;
    }
    void gamma_correct(float gamma) {
        for (int h = 0; h < res.y; h++)
            for (int w = 0; w < res.x; w++) pixels[h][w] = pow(pixels[h][w], 1 / gamma);
    }
    void save_png(std::string filename) {
        std::vector<unsigned char> data(res.x * res.y * 3);
        for (int h = 0; h < res.y; h++) {
            for (int w = 0; w < res.x; w++) {
                int index = (h * res.x + w) * 3;
                data[index + 0] =
                    static_cast<unsigned char>(clamp(pixels[res.y - h - 1][w].x, 0, 1) * 255);
                data[index + 1] =
                    static_cast<unsigned char>(clamp(pixels[res.y - h - 1][w].y, 0, 1) * 255);
                data[index + 2] =
                    static_cast<unsigned char>(clamp(pixels[res.y - h - 1][w].z, 0, 1) * 255);
            }
        }

        bool success =
            fpng::fpng_encode_image_to_file(filename.c_str(), data.data(), res.x, res.y, 3);
        if (!success) std::cerr << "Failed to write image to file: " << filename << '\n';
    }
    void save_ppm(std::string filename) {
        std::ofstream out(filename);
        out << "P6\n" << res.x << " " << res.y << "\n255\n";
        for (int h = 0; h < res.y; h++) {
            for (int w = 0; w < res.x; w++) {
                out << static_cast<unsigned char>(clamp(pixels[res.y - h - 1][w].x, 0, 1) * 255);
                out << static_cast<unsigned char>(clamp(pixels[res.y - h - 1][w].y, 0, 1) * 255);
                out << static_cast<unsigned char>(clamp(pixels[res.y - h - 1][w].z, 0, 1) * 255);
            }
        }
        out.close();
    }
};