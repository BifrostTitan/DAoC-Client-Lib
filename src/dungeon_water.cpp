#include "daoc_client_lib/dungeon_water.h"

#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string>

namespace daoc_client_lib
{
    namespace
    {
        int parse_int(const std::string &value)
        {
            int parsed = 0;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
            {
                throw std::runtime_error("Invalid integer in dungeon water CSV: " + value);
            }
            return parsed;
        }

        float parse_float(const std::string &value)
        {
            float parsed = 0.0f;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || !std::isfinite(parsed))
            {
                throw std::runtime_error("Invalid float in dungeon water CSV: " + value);
            }
            return parsed;
        }

        Vec3 parse_vec3(const CsvTable::value_type &row, std::size_t offset)
        {
            return {parse_float(row[offset]), parse_float(row[offset + 1]), parse_float(row[offset + 2])};
        }
    }

    std::map<int, DungeonWaterGeometry> parse_dungeon_water_definitions(const CsvTable &rows)
    {
        std::map<int, DungeonWaterGeometry> definitions;
        for (const auto &row : rows)
        {
            if (row.size() < 16)
            {
                continue;
            }

            DungeonWaterGeometry water;
            water.id = parse_int(row[0]);
            for (std::size_t corner = 0; corner < water.corners.size(); ++corner)
            {
                water.corners[corner] = parse_vec3(row, 1 + corner * 3);
            }
            water.translation = parse_vec3(row, 13);
            if (!definitions.emplace(water.id, water).second)
            {
                throw std::runtime_error("Duplicate dungeon water ID");
            }
        }
        return definitions;
    }

    std::map<int, int> parse_dungeon_water_locations(const CsvTable &rows)
    {
        std::map<int, int> locations;
        for (const auto &row : rows)
        {
            if (row.size() < 2)
            {
                continue;
            }
            const auto place_id = parse_int(row[0]);
            const auto water_id = parse_int(row[1]);
            if (!locations.emplace(place_id, water_id).second)
            {
                throw std::runtime_error("Duplicate dungeon water location");
            }
        }
        return locations;
    }
}