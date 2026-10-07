#pragma once

#include "daoc_client_lib/pcx_image.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace daoc_client_lib
{
    class TerrainHeightMap
    {
    public:
        static constexpr std::uint8_t terrain_factor = 255;
        static constexpr float unit_factor = terrain_factor / 65536.0f;

        TerrainHeightMap(
            const PcxImage& terrain,
            const PcxImage& offset,
            std::int16_t terrain_scale,
            std::int16_t terrain_offset);

        std::size_t width() const noexcept;
        std::size_t height() const noexcept;
        float at(std::size_t x, std::size_t y) const;
        float height_at(float game_x, float game_y) const;

    private:
        std::size_t width_ = 0;
        std::size_t height_ = 0;
        std::vector<float> samples_;
    };
}