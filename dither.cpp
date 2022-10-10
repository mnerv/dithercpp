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
#include <stdexcept>
#include <thread>
#include <chrono>

#include <cstdint>
#include <cmath>

#include "stb_image.h"
#include "stb_image_write.h"

#include "glm/glm.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#define ASIO_STANDALONE
#include "asio.hpp"

namespace nrv {
class image {
  public:
    image(std::filesystem::path const& filename) {
        using namespace std::string_literals;
        auto data = stbi_load(filename.c_str(), &m_width, &m_height, &m_channels, 0);
        if (data == nullptr)
            throw std::runtime_error("nrv::image: error reading file: \""s + filename.string() + "\""s);
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
        for (auto i = 0; i < m_height / 2; i++) {
            for (auto j = 0; j < m_width; j++) {
                auto const a = get_pixel_rgba(j, i);
                auto const b = get_pixel_rgba(j, m_height - 1 - i);
                set_pixel(j, i, b);
                set_pixel(j, m_height - 1 - i, a);
            }
        }
    }
    auto fliph() -> void {
       for (auto i = 0; i < m_height; i++) {
            for (auto j = 0; j < m_width / 2; j++) {
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
    auto buffer()   const -> float* { return m_buffer; }

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
    std::memcpy(destination.buffer(), source.buffer(), source.size() * sizeof(float));

    nrv::render_img(destination, [&](auto const& pos, auto const& pixel) {
        auto qp = quantise_fn(pixel);
        auto err = pixel - qp;
        destination.set_pixel(pos.x, pos.y, qp);

        auto update_pixel = [&](glm::i32vec2 const& offset, float const& err_bias) {
            glm::vec4 const p = destination.get_pixel_rgba(pos.x + offset.x, pos.y + offset.y);
            auto const k = p + err * err_bias;
            destination.set_pixel(pos.x + offset.x, pos.y + offset.y, {k.r, k.g, k.b, 1.0f});
        };

        // Applies the kernel
        update_pixel({+1,  0}, 7.0f / 16.0f);
        update_pixel({-1, +1}, 3.0f / 16.0f);
        update_pixel({ 0, +1}, 5.0f / 16.0f);
        update_pixel({+1, +1}, 1.0f / 16.0f);
    });
}

auto dither_minimized_average_error(nrv::image const& source, nrv::image& out, std::function<glm::vec4(glm::vec4 const& pixel)> const& quantise_fn) {
    std::memcpy(out.buffer(), source.buffer(), source.size() * sizeof(float));
    nrv::render_img(out, [&](auto const& pos, auto const& pixel) {
        auto qp = quantise_fn(pixel);
        auto err = pixel - qp;
        out.set_pixel(pos.x, pos.y, qp);

        auto update_pixel = [&](glm::i32vec2 const& offset, float const& err_bias) {
            glm::vec4 const p = out.get_pixel_rgba(pos.x + offset.x, pos.y + offset.y);
            auto const k = p + err * err_bias;
            out.set_pixel(pos.x + offset.x, pos.y + offset.y, {k.r, k.g, k.b, 1.0f});
        };

        // Applies the kernel
        update_pixel({+1,  0}, 7.0f / 48.0f);
        update_pixel({+2,  0}, 5.0f / 48.0f);
        update_pixel({-2, +1}, 3.0f / 48.0f);
        update_pixel({-1, +1}, 5.0f / 48.0f);
        update_pixel({ 0, +1}, 7.0f / 48.0f);
        update_pixel({+1, +1}, 5.0f / 48.0f);
        update_pixel({+2, +1}, 3.0f / 48.0f);
        update_pixel({-2, +2}, 1.0f / 48.0f);
        update_pixel({-1, +2}, 3.0f / 48.0f);
        update_pixel({ 0, +2}, 5.0f / 48.0f);
        update_pixel({+1, +2}, 3.0f / 48.0f);
        update_pixel({+2, +2}, 1.0f / 48.0f);
    });
}

auto main([[maybe_unused]]int argc, [[maybe_unused]]char const* argv[]) -> int {
    if (argc < 2) {
        std::cerr << "error no file given!\n\n";
        std::cerr << "usage: " << argv[0] << " [filename]\n";
        std::cerr << "    [filename] - path to image file, supported (jpg, png, or stb_image supported type)\n";
        return 1;
    }

    std::string filename = argv[1];
    if (!std::filesystem::exists(filename)) {
        std::cerr << "file: \"" << filename << "\" does not exists\n";
        return 1;
    }

    nrv::image img{filename};
    nrv::image quantised{img.width(), img.height(), img.channels()};
    nrv::image dithered{img.width(), img.height(), img.channels()};

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
    //dither_minimized_average_error(img, dithered, quantise_greyscale_1bit);

    nrv::write_png("greyscale.png", img);
    nrv::write_png("quantise.png", quantised);
    nrv::write_png("dithered.png", dithered);

    if (argc < 3) return 0;
    std::string ip = argv[2];

    asio::error_code ec;
    asio::io_context io;
    asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip, ec), 80);
    asio::ip::tcp::socket socket(io);
    socket.connect(endpoint, ec);
    if (ec) {
        std::cerr << "Error connecting...\n";
        return 1;
    }

    if (socket.is_open()) {
        auto size = static_cast<std::size_t>(dithered.width() * dithered.height());
        auto data = new std::uint8_t[size];
        for (auto i = 0; i < dithered.height(); ++i) {
            for (auto j = 0; j < dithered.width(); ++j) {
                data[i * dithered.width() + j] = std::uint8_t(dithered.get_pixel_rgb(j, i).r * 255.0f);
            }
        }
        socket.write_some(asio::buffer(data, size));
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(250ms);
        delete[] data;
    }

    return 0;
}

