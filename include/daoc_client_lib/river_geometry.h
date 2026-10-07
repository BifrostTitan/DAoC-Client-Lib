#pragma once

#include "daoc_client_lib/text_data.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace daoc_client_lib
{
    struct RiverBank
    {
        std::array<std::int16_t, 3> left{};
        std::array<std::int16_t, 3> right{};
    };

    struct RiverGeometry
    {
        int id = 0;
        std::optional<std::string> name;
        std::optional<std::string> type;
        std::optional<std::string> texture;
        std::optional<std::string> multitexture;
        std::int16_t flow = 0;
        int height = -1;
        int color = 0;
        int extend_pos_x = 0;
        int extend_pos_y = 0;
        int extend_neg_x = 0;
        int extend_neg_y = 0;
        std::int16_t tessellation = 0;
        std::vector<RiverBank> banks;
    };

    std::vector<RiverGeometry> parse_rivers(const DatDocument& sector_data);
}