/**
 * @file   image.hpp
 * @author mononerv (me@mononerv.dev)
 * @brief  image library
 * @date   2022-10-14
 *
 * @copyright Copyright (c) 2022 mononerv
 */
#ifndef IMAGEPP_IMAGE_HPP
#define IMAGEPP_IMAGE_HPP

#include <cstdint>
#include <filesystem>
#include <string>
#include <functional>

#include "glm/glm.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

namespace nrv {
class image {
  public:
    image(std::filesystem::path const& filename);
    image(std::int32_t const& size);
    image(std::int32_t const& width, std::int32_t const& height, std::int32_t const& channels = 3);
    ~image();

    auto width()    const -> std::int32_t { return m_width; }
    auto height()   const -> std::int32_t { return m_height; }
    auto channels() const -> std::int32_t { return m_channels; }
    auto size()     const -> std::size_t  { return m_size; }
    auto buffer()   const -> float*       { return m_buffer; }
    auto str()      const -> std::string;

  public:
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

  public:
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

  private:
    std::filesystem::path m_filename{""};
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
auto write_png(std::string const& filename, image const& img) -> void;

using render_fn_t     = std::function<glm::vec4(glm::i32vec2 const& pos)>;
using sample_fn_t     = std::function<glm::vec4(glm::i32vec2 const& pos, glm::vec4 const& pixel)>;
using transform_fn_t  = std::function<glm::vec4(glm::vec4 const& pixel)>;
using render_set_fn_t = std::function<void(glm::i32vec2 const& pos, glm::vec4 const& pixel)>;
auto render_img(image& img, render_fn_t const& fn) -> void;
auto render_img(image& img, sample_fn_t const& fn) -> void;
auto render_img(image const& img, render_set_fn_t const& fn) -> void;
auto render_transform(image const& source, image& output, transform_fn_t const& fn) -> void;
auto render_transform(image const& source, image& output, sample_fn_t const& fn) -> void;
}

#endif  // IMAGEPP_IMAGE_HPP
