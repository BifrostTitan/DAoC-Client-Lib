#pragma once

#include "daoc_client_lib/text_data.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace daoc_client_lib
{
    class ClientDataDirectory;

    struct ZoneData
    {
        std::string zone_header;
        bool enabled = false;
        std::int16_t id = 0;
        std::int16_t type = -1;
        std::int16_t region = -1;
        std::optional<std::string> name;
        std::int16_t offset_x = -1;
        std::int16_t offset_y = -1;
        std::int16_t width = -1;
        std::int16_t height = -1;
        std::int16_t temperature = INT16_MIN;
        std::int16_t entry_music = -1;
        std::optional<std::string> sky_dome;
        bool map_enabled = true;
        std::int16_t proxy_zone = -1;

        bool is_proxy_zone() const noexcept;
    };

    std::vector<ZoneData> parse_zones(const DatDocument& document);
    std::vector<ZoneData> read_zones(const ClientDataDirectory& client);
}