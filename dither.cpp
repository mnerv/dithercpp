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

#define ASIO_STANDALONE
#include "asio.hpp"

#include "image.hpp"

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

