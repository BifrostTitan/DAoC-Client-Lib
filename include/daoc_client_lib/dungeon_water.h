#pragma once

#include "daoc_client_lib/text_data.h"

#include <array>
#include <map>

namespace daoc_client_lib
{
    struct Vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct DungeonWaterGeometry
    {
        int id = 0;
        std::array<Vec3, 4> corners{};
        Vec3 translation{};
    };

    std::map<int, DungeonWaterGeometry> parse_dungeon_water_definitions(const CsvTable& rows);
    std::map<int, int> parse_dungeon_water_locations(const CsvTable& rows);
}