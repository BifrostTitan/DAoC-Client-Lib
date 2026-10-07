#include "daoc_client_lib/jump_point.h"

#include "daoc_client_lib/client_data_directory.h"

#include <charconv>
#include <limits>
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
                throw std::runtime_error("Invalid jump point integer: " + value);
            }
            return parsed;
        }

        bool try_parse_int(const std::string &value, int &parsed)
        {
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            return result.ec == std::errc{} && result.ptr == value.data() + value.size();
        }

        std::string bytes_to_string(const std::vector<std::uint8_t> &bytes)
        {
            return {bytes.begin(), bytes.end()};
        }
    }

    std::vector<JumpPoint> parse_jump_points(const CsvTable &rows)
    {
        std::vector<JumpPoint> points;
        for (const auto &row : rows)
        {
            if (row.size() <= 6)
            {
                throw std::runtime_error("Jump point row has fewer than seven fields");
            }
            const auto heading = parse_int(row[6]);
            if (heading < std::numeric_limits<std::int16_t>::min() || heading > std::numeric_limits<std::int16_t>::max())
            {
                throw std::out_of_range("Jump point heading is outside 16-bit range");
            }

            JumpPoint point;
            point.id = parse_int(row[0]);
            point.name = row[1];
            point.zone_id = parse_int(row[2]);
            point.x = parse_int(row[3]);
            point.y = parse_int(row[4]);
            point.z = parse_int(row[5]);
            point.heading_degrees = static_cast<std::int16_t>(heading);
            for (std::size_t index = 7; index < row.size(); ++index)
            {
                int mirror_zone = 0;
                if (try_parse_int(row[index], mirror_zone) && mirror_zone > 0)
                {
                    point.also_in_zone_ids.push_back(mirror_zone);
                }
            }
            points.push_back(std::move(point));
        }
        return points;
    }

    std::vector<JumpPoint> read_jump_points(const ClientDataDirectory &client)
    {
        const auto bytes = client.read_package_file("gamedata.mpk", "jumppoints.csv");
        return parse_jump_points(parse_csv(bytes_to_string(bytes)));
    }
}