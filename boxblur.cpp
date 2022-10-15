/**
 * @file   gaussian.cpp
 * @author mononerv (me@mononerv.dev)
 * @brief  Box blur
 *         https://en.wikipedia.org/wiki/Box_blur
 * @date   2022-10-14
 *
 * @copyright Copyright (c) 2022 mononerv
 */
#include <random>
#include <filesystem>
#include <cmath>
#include <iostream>

#include "image.hpp"

auto box_blur(nrv::image const& img) -> nrv::image {
    nrv::image output{img.width(), img.height(), img.channels()};
    nrv::render_img(img, [&](glm::ivec2 const& pos, glm::vec4 const&) {
        auto sum_at = [&](glm::ivec2 const& offset) {
            return img.get_pixel_rgba(pos.x + offset.x, pos.y + offset.y);
        };

        std::int32_t const blur_width  = 1;
        std::int32_t const blur_height = 1;
        auto sum = glm::vec4{0.0f};
        auto denom = 0.0f;
        for (std::int32_t i = -blur_height; i <= blur_height; ++i) {
            for (std::int32_t j = -blur_width; j <= blur_width; ++j) {
                sum += sum_at({j, i});
                denom += 1.0f;
            }
        }

        output.set_pixel(pos.x, pos.y, sum / denom);
    });
    return output;
}

auto main([[maybe_unused]]int argc, [[maybe_unused]]char const* argv[]) -> int {
    if (argc < 2) {
        std::cout << "No file given\n";
        std::cout << "usage: " << argv[0] << " {filename}\n";
        return 1;
    }

    std::filesystem::path filename = argv[1];
    if (!std::filesystem::exists(filename)) {
        std::cout << "Not a valid file\n";
        return 1;
    }

    nrv::image image{filename};
    auto out = box_blur(image);
    nrv::write_png("box_blur_out.png", out);

    return 0;
}
