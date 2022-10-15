/**
 * @file   image.cpp
 * @author mononerv (me@mononerv.dev)
 * @brief  image library
 * @date   2022-10-14
 *
 * @copyright Copyright (c) 2022 mononerv
 */
#include "image.hpp"

#include <algorithm>

#include "stb_image.h"
#include "stb_image_write.h"

namespace nrv {
image::image(std::filesystem::path const& filename) : m_filename(filename) {
    using namespace std::string_literals;
    auto data = stbi_load(m_filename.string().c_str(), &m_width, &m_height, &m_channels, 0);
    if (data == nullptr)
        throw std::runtime_error("nrv::image: error reading file: \""s + filename.string() + "\""s);
    m_size = static_cast<std::size_t>(m_width * m_height * m_channels);
    m_buffer = new float[m_size];
    std::transform(data, data + m_size, m_buffer, [](auto const& pixel) {
        return static_cast<float>(pixel) / 255.0f;
    });
    stbi_image_free(data);
}
image::image(std::int32_t const& size) : image(size, size) {}
image::image(std::int32_t const& width, std::int32_t const& height, std::int32_t const& channels)
    : m_width(width), m_height(height), m_channels(channels)
    , m_size(static_cast<std::size_t>(m_width * m_height * m_channels))
    , m_buffer(new float[m_size]) {}
image::~image() {
    delete[] m_buffer;
}
auto image::str()  const -> std::string {
    std::string str{"nrv::image{"};
    str += "file: \""   + m_filename.string()        + "\", ";
    str += "width: "    + std::to_string(m_width)    + ", ";
    str += "height: "   + std::to_string(m_height)   + ", ";
    str += "channels: " + std::to_string(m_channels) + ", ";
    str += "size: "     + std::to_string(m_size);
    str += "}";
    return str;
}

auto write_png(std::string const& filename, image const& img) -> void {
    auto data = new std::uint8_t[img.size()];
    std::transform(img.buffer(), img.buffer() + img.size(), data, [](auto const& pixel) {
        return std::uint8_t(std::clamp(pixel * 255.0f, 0.0f, 255.0f));
    });
    stbi_write_png(filename.c_str(), img.width(), img.height(), img.channels(), data, img.width() * img.channels());
    delete[] data;
}

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
auto render_transform(image const& source, image& output, transform_fn_t const& fn) -> void {
    for (std::int32_t i = 0; i < source.height(); i++)
        for (std::int32_t j = 0; j < source.width(); j++)
            output.set_pixel(j, i, fn(source.get_pixel_rgba(j, i)));
}
auto render_transform(image const& source, image& output, sample_fn_t const& fn) -> void {
    for (std::int32_t i = 0; i < source.height(); i++)
        for (std::int32_t j = 0; j < source.width(); j++)
            output.set_pixel(j, i, fn({j, i}, source.get_pixel_rgba(j, i)));
}
}

