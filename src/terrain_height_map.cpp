#include "daoc_client_lib/terrain_height_map.h"

#include <cmath>
#include <stdexcept>

namespace daoc_client_lib
{
    namespace
    {
        struct Point3
        {
            float x;
            float height;
            float y;
        };

        float interpolate_triangle(const Point3 &a, const Point3 &b, const Point3 &c, float x, float y)
        {
            const float determinant = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
            const float weight_a = ((b.y - c.y) * (x - c.x) + (c.x - b.x) * (y - c.y)) / determinant;
            const float weight_b = ((c.y - a.y) * (x - c.x) + (a.x - c.x) * (y - c.y)) / determinant;
            const float weight_c = 1.0f - weight_a - weight_b;
            return weight_a * a.height + weight_b * b.height + weight_c * c.height;
        }
    }

    TerrainHeightMap::TerrainHeightMap(
        const PcxImage &terrain,
        const PcxImage &offset,
        std::int16_t terrain_scale,
        std::int16_t terrain_offset)
        : width_(terrain.width()), height_(terrain.height())
    {
        if (width_ != offset.width() || height_ != offset.height())
        {
            throw std::invalid_argument("Terrain and offset PCX dimensions must match");
        }
        samples_.resize(width_ * height_);
        for (std::size_t y = 0; y < height_; ++y)
        {
            for (std::size_t x = 0; x < width_; ++x)
            {
                const float terrain_value = terrain.pixel(x, y).red / static_cast<float>(terrain_factor) * terrain_scale;
                const float offset_value = offset.pixel(x, y).red / static_cast<float>(terrain_factor) * terrain_offset;
                samples_[y * width_ + x] = terrain_value + offset_value;
            }
        }
    }

    std::size_t TerrainHeightMap::width() const noexcept
    {
        return width_;
    }

    std::size_t TerrainHeightMap::height() const noexcept
    {
        return height_;
    }

    float TerrainHeightMap::at(std::size_t x, std::size_t y) const
    {
        if (x >= width_ || y >= height_)
        {
            throw std::out_of_range("Terrain sample coordinates are outside the height map");
        }
        return samples_[y * width_ + x];
    }

    float TerrainHeightMap::height_at(float game_x, float game_y) const
    {
        if (!std::isfinite(game_x) || !std::isfinite(game_y) || game_x < 0.0f || game_y < 0.0f)
        {
            throw std::out_of_range("Terrain coordinates must be finite and non-negative");
        }

        const float x = game_x * unit_factor;
        const float y = game_y * unit_factor;
        if (!std::isfinite(x) || !std::isfinite(y) || width_ < 2 || height_ < 2 || x >= static_cast<float>(width_ - 1) || y >= static_cast<float>(height_ - 1))
        {
            throw std::out_of_range("Terrain coordinates are outside the interpolable map area");
        }
        const auto cell_x = static_cast<std::size_t>(std::floor(x));
        const auto cell_y = static_cast<std::size_t>(std::floor(y));
        if (cell_x + 1 >= width_ || cell_y + 1 >= height_)
        {
            throw std::out_of_range("Terrain coordinates are outside the interpolable map area");
        }

        const auto sample = [this](std::size_t sample_x, std::size_t sample_y)
        {
            return at(sample_x, sample_y);
        };
        const Point3 top_right{static_cast<float>(cell_x + 1), sample(cell_x + 1, cell_y + 1),
                               static_cast<float>(cell_y + 1)};
        Point3 b{};
        Point3 c{};
        if (x - cell_x < y - cell_y)
        {
            b = {static_cast<float>(cell_x + 1), sample(cell_x + 1, cell_y), static_cast<float>(cell_y)};
            c = {static_cast<float>(cell_x), sample(cell_x, cell_y), static_cast<float>(cell_y)};
        }
        else
        {
            b = {static_cast<float>(cell_x), sample(cell_x, cell_y), static_cast<float>(cell_y)};
            c = {static_cast<float>(cell_x), sample(cell_x, cell_y + 1), static_cast<float>(cell_y + 1)};
        }

        return interpolate_triangle(top_right, b, c, x, y) / unit_factor;
    }
}