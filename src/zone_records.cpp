#include "daoc_client_lib/zone_records.h"

#include <algorithm>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <utility>

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
                throw std::runtime_error("Invalid integer in zone CSV: " + value);
            }
            return parsed;
        }

        std::int16_t parse_short(const std::string &value)
        {
            const auto parsed = parse_int(value);
            if (parsed < std::numeric_limits<std::int16_t>::min() || parsed > std::numeric_limits<std::int16_t>::max())
            {
                throw std::out_of_range("Zone CSV value is outside 16-bit range");
            }
            return static_cast<std::int16_t>(parsed);
        }
    }

    std::map<std::int16_t, ZoneJump> parse_zone_jumps(const CsvTable &rows)
    {
        std::map<std::int16_t, ZoneJump> jumps;
        for (const auto &row : rows)
        {
            if (row.size() < 9)
            {
                continue;
            }
            ZoneJump jump;
            jump.order = parse_short(row[0]);
            jump.name = row[1];
            jump.x1 = parse_int(row[2]);
            jump.y1 = parse_int(row[3]);
            jump.x2 = parse_int(row[4]);
            jump.y2 = parse_int(row[5]);
            jump.z1 = parse_int(row[6]);
            jump.z2 = parse_int(row[7]);
            jump.id = parse_short(row[8]);
            if (!jumps.emplace(jump.id, std::move(jump)).second)
            {
                throw std::runtime_error("Duplicate zone jump ID");
            }
        }
        return jumps;
    }

    ZoneBounds parse_zone_bounds(const CsvTable &rows)
    {
        ZoneBounds bounds;
        for (const auto &row : rows)
        {
            if (row.size() < 2)
            {
                continue;
            }

            const auto declared_points = parse_int(row[1]);
            const auto available_values = row.size() - 2;
            const auto value_count = declared_points < 0
                                         ? std::size_t{0}
                                         : std::min(available_values, static_cast<std::size_t>(declared_points) > available_values / 2
                                                                          ? available_values
                                                                          : static_cast<std::size_t>(declared_points) * 2);
            std::vector<ZonePoint> points;
            points.reserve(value_count / 2);
            for (std::size_t index = 2; index + 1 < 2 + value_count; index += 2)
            {
                points.push_back({parse_int(row[index]), parse_int(row[index + 1])});
            }
            bounds.push_back(std::move(points));
        }
        return bounds;
    }
}