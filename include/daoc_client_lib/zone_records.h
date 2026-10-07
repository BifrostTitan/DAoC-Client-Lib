#pragma once

#include "daoc_client_lib/text_data.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace daoc_client_lib
{
    struct ZoneJump
    {
        std::int16_t id = 0;
        std::int16_t order = 0;
        std::string name;
        int x1 = 0;
        int y1 = 0;
        int x2 = 0;
        int y2 = 0;
        int z1 = 0;
        int z2 = 0;
    };

    struct ZonePoint
    {
        int x = 0;
        int y = 0;
    };

    using ZoneBounds = std::vector<std::vector<ZonePoint>>;

    std::map<std::int16_t, ZoneJump> parse_zone_jumps(const CsvTable& rows);
    ZoneBounds parse_zone_bounds(const CsvTable& rows);
}