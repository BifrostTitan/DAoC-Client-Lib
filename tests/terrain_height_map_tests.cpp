#include "daoc_client_lib/terrain_height_map.h"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace
{
    void check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void write_u16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value)
    {
        bytes[offset] = static_cast<std::uint8_t>(value);
        bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8);
    }

    daoc_client_lib::PcxImage indexed_image(const std::array<std::uint8_t, 4>& pixels)
    {
        std::vector<std::uint8_t> bytes(128, 0);
        bytes[0] = 0x0a;
        bytes[1] = 5;
        bytes[2] = 0;
        bytes[3] = 8;
        write_u16(bytes, 8, 1);
        write_u16(bytes, 10, 1);
        bytes[65] = 1;
        write_u16(bytes, 66, 2);
        bytes.insert(bytes.end(), pixels.begin(), pixels.end());
        bytes.push_back(0x0c);
        bytes.resize(bytes.size() + 256 * 3, 0);
        for (std::size_t index = 0; index < 4; ++index)
        {
            bytes[131 + index * 3] = static_cast<std::uint8_t>(index);
        }
        return daoc_client_lib::PcxImage::decode(bytes);
    }

    void test_height_map_and_interpolation()
    {
        const auto terrain = indexed_image({0, 1, 2, 3});
        const auto offset = indexed_image({0, 0, 0, 0});
        const daoc_client_lib::TerrainHeightMap map(terrain, offset, 255, 0);
        check(map.width() == 2 && map.height() == 2, "terrain map dimensions mismatch");
        check(map.at(1, 1) == 3.0f, "terrain red-channel conversion mismatch");

        const auto sampled = map.height_at(0.25f / daoc_client_lib::TerrainHeightMap::unit_factor,
                                           0.5f / daoc_client_lib::TerrainHeightMap::unit_factor);
        check(sampled > 1.24f && sampled < 1.26f, "terrain triangle interpolation mismatch");
    }
}

int run_terrain_height_map_tests()
{
    test_height_map_and_interpolation();
    return 0;
}