#pragma once

#include "daoc_client_lib/text_data.h"

#include <cstdint>
#include <vector>

namespace daoc_client_lib
{
    class ClientDataDirectory;

    struct JumpPoint
    {
        int id = 0;
        std::string name;
        int zone_id = 0;
        int x = 0;
        int y = 0;
        int z = 0;
        std::int16_t heading_degrees = 0;
        std::vector<int> also_in_zone_ids;
    };

    std::vector<JumpPoint> parse_jump_points(const CsvTable& rows);
    std::vector<JumpPoint> read_jump_points(const ClientDataDirectory& client);
}