#include "daoc_client_lib/zone_data.h"

#include "daoc_client_lib/client_data_directory.h"

#include <charconv>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace daoc_client_lib
{
    namespace
    {
        int parse_int(const std::string &value, const std::string &field_name)
        {
            int parsed = 0;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
            {
                throw std::runtime_error("Invalid integer for zone field " + field_name + ": " + value);
            }
            return parsed;
        }

        std::int16_t parse_short_value(const std::string &value, const char *key)
        {
            const auto parsed = parse_int(value, key);
            if (parsed < std::numeric_limits<std::int16_t>::min() || parsed > std::numeric_limits<std::int16_t>::max())
            {
                throw std::out_of_range("Zone field is outside 16-bit range: " + std::string(key));
            }
            return static_cast<std::int16_t>(parsed);
        }

        std::int16_t parse_short(const DatSection &section, const char *key, std::int16_t fallback)
        {
            const auto field = section.find(key);
            return field == section.end() ? fallback : parse_short_value(field->second, key);
        }

        bool is_zone_header(std::string_view header)
        {
            if (header.size() <= 4 || header.size() > 7 || header.substr(0, 4) != "zone")
            {
                return false;
            }
            for (const auto character : header.substr(4))
            {
                if (character < '0' || character > '9')
                {
                    return false;
                }
            }
            return true;
        }

        ZoneData parse_zone(const std::string &header, const DatSection &section)
        {
            ZoneData zone;
            zone.zone_header = header;
            zone.id = parse_short_value(header.substr(4), "id");

            const auto enabled = section.find("enabled");
            if (enabled != section.end())
            {
                zone.enabled = parse_int(enabled->second, "enabled") > 0;
            }
            zone.type = parse_short(section, "type", zone.type);
            zone.region = parse_short(section, "region", zone.region);
            zone.offset_x = parse_short(section, "region_offset_x", zone.offset_x);
            zone.offset_y = parse_short(section, "region_offset_y", zone.offset_y);
            zone.width = parse_short(section, "width", zone.width);
            zone.height = parse_short(section, "height", zone.height);
            zone.temperature = parse_short(section, "temperature", zone.temperature);
            zone.entry_music = parse_short(section, "entry_music", zone.entry_music);
            zone.proxy_zone = parse_short(section, "proxy_zone", zone.proxy_zone);

            const auto name = section.find("name");
            if (name != section.end())
            {
                zone.name = name->second;
            }
            const auto sky_dome = section.find("skydome");
            if (sky_dome != section.end())
            {
                zone.sky_dome = sky_dome->second;
            }
            const auto map_enabled = section.find("map_enabled");
            if (map_enabled != section.end())
            {
                zone.map_enabled = parse_int(map_enabled->second, "map_enabled") > 0;
            }
            return zone;
        }
    }

    bool ZoneData::is_proxy_zone() const noexcept
    {
        return proxy_zone > -1;
    }

    std::vector<ZoneData> parse_zones(const DatDocument &document)
    {
        std::vector<ZoneData> zones;
        for (const auto &[header, section] : document)
        {
            if (is_zone_header(header))
            {
                zones.push_back(parse_zone(header, section));
            }
        }
        return zones;
    }

    std::vector<ZoneData> read_zones(const ClientDataDirectory &client)
    {
        const auto bytes = client.read_package_file("zones.mpk", "zones.dat");
        const std::string content(bytes.begin(), bytes.end());
        return parse_zones(parse_dat(content));
    }
}