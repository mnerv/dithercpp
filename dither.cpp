/**
 * @file   dither.cpp
 * @author mononerv (me@mononerv.dev)
 * @brief  Floyd–Steinberg dithering in C++
 * @date   2022-10-02
 *
 * @copyright Copyright (c) 2022 mononerv
 */
#include <iostream>
#include <algorithm>
#include <numbers>
#include <vector>
#include <stack>
#include <functional>
#include <random>
#include <filesystem>

#include <cstdint>
#include <cmath>

#include "stb_image.h"
#include "stb_image_write.h"

#include "glm/glm.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace nrv {
class image {
  public:
    image(std::filesystem::path const& filename) {
        auto data = stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, 0);
        m_size = static_cast<std::size_t>(m_width * m_height * m_channels);
        m_buffer = new float[m_size];
        std::transform(data, data + m_size, m_buffer, [](auto const& pixel) {
            return static_cast<float>(pixel) / 255.0f;
        });
        stbi_image_free(data);
    }
    image(std::int32_t const& size) : image(size, size) {}
    image(std::int32_t const& width, std::int32_t const& height, std::int32_t const& channels = 3)
        : m_width(width), m_height(height), m_channels(channels)
        , m_size(static_cast<std::size_t>(m_width * m_height * m_channels))
        , m_buffer(new float[m_size]) {}
    ~image() {
        delete[] m_buffer;
    }

    auto set_pixel(std::int32_t const& x, std::int32_t const& y, glm::vec3 const& color) -> void {
        if (x < 0 || x > m_width - 1 || y < 0 || y > m_height - 1) return;
        auto const index = (y * m_channels) * m_width + (x * m_channels);
        m_buffer[index + 0] = color.r;
        m_buffer[index + 1] = color.g;
        m_buffer[index + 2] = color.b;
    }
    auto set_pixel(std::int32_t const& x, std::int32_t const& y, glm::vec4 const& color) -> void {
        if (x < 0 || x > m_width - 1 || y < 0 || y > m_height - 1) return;
        set_pixel(x, y, {color.r, color.g, color.b});
        auto const index = (y * m_channels) * m_width + (x * m_channels);
        if (m_channels == 4) m_buffer[index + 3] = color.a;
    }

    auto get_pixel_rgb(std::int32_t const& x, std::int32_t const& y) const -> glm::vec3 {
        if (x < 0 || x > m_width - 1 || y < 0 || y > m_height - 1) return {0.0f, 0.0f, 0.0f};
        auto const index = (y * m_channels) * m_width + (x * m_channels);
        return {
            m_buffer[index + 0],
            m_buffer[index + 1],
            m_buffer[index + 2]
        };
    }
    auto get_pixel_rgba(std::int32_t const& x, std::int32_t const& y) const -> glm::vec4 {
        if (x < 0 || x > m_width - 1 || y < 0 || y > m_height - 1) return {0.0f, 0.0f, 0.0f, 0.0f};
        auto const index = (y * m_channels) * m_width + (x * m_channels);
        auto alpha = m_channels == 4 ? m_buffer[index + 3] : 1.0f;
        return {
            m_buffer[index + 0],
            m_buffer[index + 1],
            m_buffer[index + 2],
            alpha
        };
    }
    auto flipv() -> void {
        for (std::int32_t i = 0; i < m_height / 2; i++) {
            for (std::int32_t j = 0; j < m_width; j++) {
                auto const a = get_pixel_rgba(j, i);
                auto const b = get_pixel_rgba(j, m_height - 1 - i);
                set_pixel(j, i, b);
                set_pixel(j, m_height - 1 - i, a);
            }
        }
    }
    auto fliph() -> void {
       for (int32_t i = 0; i < m_height; i++) {
            for (int32_t j = 0; j < m_width / 2; j++) {
                const auto a = get_pixel_rgba(j, i);
                const auto b = get_pixel_rgba(m_width - 1 - j, i);
                set_pixel(j, i, b);
                set_pixel(m_width - 1 - j, i, a);
            }
        }
    }
    auto normalise() -> void {
        auto max = *std::max_element(m_buffer, m_buffer + m_size);
        std::transform(m_buffer, m_buffer + m_size, m_buffer, [&max](auto const& value) {
            return value / max;
        });
    }

    auto width()    const -> std::int32_t { return m_width; }
    auto height()   const -> std::int32_t { return m_height; }
    auto channels() const -> std::int32_t { return m_channels; }
    auto size()     const -> std::size_t  { return m_size; }
    auto buffer()   const -> float const* { return m_buffer; }

  private:
    std::int32_t m_width;
    std::int32_t m_height;
    std::int32_t m_channels;
    std::size_t  m_size;
    float*       m_buffer;
};

/**
 * Convert to 8-bit pixel data and save as PNG file
 * @param filename Location to store the image file.
 * @param img      Image data to convert and save.
 */
auto write_png(std::string const& filename, image const& img) -> void {
    auto data = new std::uint8_t[img.size()];
    std::transform(img.buffer(), img.buffer() + img.size(), data, [](auto const& pixel) {
        return std::uint8_t(std::clamp(pixel * 255.0f, 0.0f, 255.0f));
    });
    stbi_write_png(filename.c_str(), img.width(), img.height(), img.channels(), data, img.width() * img.channels());
    delete[] data;
}

auto line(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1, image& img, glm::vec3 const& color) -> void {
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    std::int32_t dx = x1 - x0;
    std::int32_t dy = y1 - y0;
    std::int32_t derror2 = std::abs(dy) * 2;
    std::int32_t error2  = 0;
    std::int32_t y = y0;
    for (std::int32_t x = x0; x <= x1; x++) {
        if (steep)
            img.set_pixel(y, x, color);
        else
            img.set_pixel(x, y, color);
        error2 += derror2;
        if (error2 > dx) {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}
auto line(glm::i32vec2 const& t0, glm::i32vec2 const& t1, image& img, glm::vec3 const& color) -> void {
    line(t0.x, t0.y, t1.x, t1.y, img, color);
}

auto triangle(glm::i32vec2 t0, glm::i32vec2 t1, glm::i32vec2 t2, image& img, [[maybe_unused]]glm::vec3 const& color) -> void {
    if (t0.y == t1.y && t0.y == t2.y) return;
    if (t0.y > t1.y) std::swap(t0, t1);
    if (t0.y > t2.y) std::swap(t0, t2);
    if (t1.y > t2.y) std::swap(t1, t2);
    std::int32_t total_height = t2.y - t0.y;
    for (std::int32_t i = 0; i < total_height; i++) {
        bool second_half = i > t1.y - t0.y || t1.y == t0.y;
        auto segement_height = second_half ? t2.y - t1.y : t1.y - t0.y;
        float alpha = float(i) / float(total_height);
        float beta  = float(i - (second_half ? t1.y - t0.y : 0)) / float(segement_height);
        glm::vec2 A = glm::vec2(t0) + glm::vec2(t2 - t0) * alpha;
        glm::vec2 B = second_half ?
                      glm::vec2(t1) + glm::vec2(t2 - t1) * beta
                      :
                      glm::vec2(t0) + glm::vec2(t1 - t0) * beta;
        if (A.x > B.x) std::swap(A, B);
        for (float j = A.x; j <= B.x; j++)
            img.set_pixel(std::int32_t(j), t0.y + i, color);
    }
}

using render_fn_t     = std::function<glm::vec4(glm::i32vec2 const& pos)>;
using sample_fn_t     = std::function<glm::vec4(glm::i32vec2 const& pos, glm::vec4 const& pixel)>;
using transform_fn_t  = std::function<glm::vec4(glm::vec4 const& pixel)>;
using render_set_fn_t = std::function<void(glm::i32vec2 const& pos, glm::vec4 const& pixel)>;
auto render_img(image& img, render_fn_t const& fn) -> void {
    for (std::int32_t i = 0; i < img.height(); i++)
        for (std::int32_t j = 0; j < img.width(); j++)
            img.set_pixel(j, i, fn({j, i}));
}
auto render_img(image& img, sample_fn_t const& fn) -> void {
    for (std::int32_t i = 0; i < img.height(); i++)
        for (std::int32_t j = 0; j < img.width(); j++)
            img.set_pixel(j, i, fn({j, i}, img.get_pixel_rgba(j, i)));
}
auto render_img(image const& img, render_set_fn_t const& fn) -> void {
    for (std::int32_t i = 0; i < img.height(); i++)
        for (std::int32_t j = 0; j < img.width(); j++)
            fn({j, i}, img.get_pixel_rgba(j, i));
}
auto render_transform(image const& source, image& output, transform_fn_t const& fn) {
    for (std::int32_t i = 0; i < source.height(); i++)
        for (std::int32_t j = 0; j < source.width(); j++)
            output.set_pixel(j, i, fn(source.get_pixel_rgba(j, i)));
};
auto render_transform(image const& source, image& output, sample_fn_t const& fn) {
    for (std::int32_t i = 0; i < source.height(); i++)
        for (std::int32_t j = 0; j < source.width(); j++)
            output.set_pixel(j, i, fn({j, i}, source.get_pixel_rgba(j, i)));
}
}

auto dither_floyd_steinberg(nrv::image const& source, nrv::image& destination, std::function<glm::vec4(glm::vec4 const& pixel)> const& quantise_fn) {
    nrv::render_transform(source, destination, [](glm::vec4 const& pixel) { return pixel; });

    nrv::render_img(destination, [&](auto const& pos, auto const& pixel) {
        auto qp = quantise_fn(pixel);
        auto err = pixel - qp;
        destination.set_pixel(pos.x, pos.y, qp);

        auto update_pixel = [&](glm::i32vec2 const& offset, float const& err_bias) {
            glm::vec4 const p = destination.get_pixel_rgba(pos.x + offset.x, pos.y + offset.y);
            auto const k = p + err * err_bias;
            destination.set_pixel(pos.x + offset.x, pos.y + offset.y, {k.r, k.g, k.b, 1.0f});
        };

        update_pixel({+1,  0}, 7.0f / 16.0f);
        update_pixel({-1, +1}, 3.0f / 16.0f);
        update_pixel({ 0, +1}, 5.0f / 16.0f);
        update_pixel({+1, +1}, 1.0f / 16.0f);
    });
}

auto main([[maybe_unused]]int argc, [[maybe_unused]]char const* argv[]) -> int {
    nrv::image img{"/Users/k/Downloads/doraemon.jpg"};
    nrv::image quantised{img.width(), img.height()};
    nrv::image dithered{img.width(), img.height()};

    auto rgb_to_greyscale = [](glm::i32vec2 const&, glm::vec4 const& pixel) {
        float greyscale = 0.2162f * pixel.r + 0.7152f * pixel.g + 0.0722f * pixel.b;
        return glm::vec4(greyscale);
    };
    nrv::render_transform(img, img, rgb_to_greyscale);

    auto quantise_greyscale_1bit = [](glm::vec4 const& in) {
        return in.r < 0.5f ? glm::vec4{0.0f} : glm::vec4{1.0f};
    };
    nrv::render_transform(img, quantised, quantise_greyscale_1bit);
    dither_floyd_steinberg(img, dithered, quantise_greyscale_1bit);

    nrv::write_png("greyscale.png", img);
    nrv::write_png("quantise.png", quantised);
    nrv::write_png("dithered.png", dithered);
    return 0;
}

