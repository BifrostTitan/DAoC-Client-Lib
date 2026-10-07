#include "daoc_client_lib/river_geometry.h"

#include <charconv>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace daoc_client_lib
{
    namespace
    {
        int parse_int(const std::string &value, const char *field_name)
        {
            int parsed = 0;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
            {
                throw std::runtime_error("Invalid river field: " + std::string(field_name));
            }
            return parsed;
        }

        int parse_int(const DatSection &section, const char *key, int fallback)
        {
            const auto field = section.find(key);
            return field == section.end() ? fallback : parse_int(field->second, key);
        }

        std::int16_t parse_short(const DatSection &section, const char *key, std::int16_t fallback)
        {
            const auto value = parse_int(section, key, fallback);
            if (value < std::numeric_limits<std::int16_t>::min() || value > std::numeric_limits<std::int16_t>::max())
            {
                throw std::out_of_range("River field is outside 16-bit range: " + std::string(key));
            }
            return static_cast<std::int16_t>(value);
        }

        std::array<std::int16_t, 3> parse_bank_side(const std::string &value)
        {
            std::array<std::int16_t, 3> coordinates{};
            std::size_t start = 0;
            for (std::size_t index = 0; index < coordinates.size(); ++index)
            {
                const auto comma = value.find(',', start);
                const auto end = comma == std::string::npos ? value.size() : comma;
                auto begin = start;
                auto trimmed_end = end;
                while (begin < trimmed_end && std::isspace(static_cast<unsigned char>(value[begin])))
                {
                    ++begin;
                }
                while (trimmed_end > begin && std::isspace(static_cast<unsigned char>(value[trimmed_end - 1])))
                {
                    --trimmed_end;
                }
                if (begin == trimmed_end)
                {
                    throw std::runtime_error("River bank coordinate is empty");
                }

                int coordinate = 0;
                const auto result = std::from_chars(value.data() + begin, value.data() + trimmed_end, coordinate);
                if (result.ec != std::errc{} || result.ptr != value.data() + trimmed_end || coordinate < std::numeric_limits<std::int16_t>::min() || coordinate > std::numeric_limits<std::int16_t>::max())
                {
                    throw std::runtime_error("Invalid river bank coordinate");
                }
                coordinates[index] = static_cast<std::int16_t>(coordinate);
                if (index + 1 < coordinates.size() && comma == std::string::npos)
                {
                    throw std::runtime_error("River bank requires three coordinates");
                }
                start = comma == std::string::npos ? end : comma + 1;
            }
            return coordinates;
        }

        std::string numbered_key(const char *prefix, int index)
        {
            std::ostringstream key;
            key << prefix << std::setw(2) << std::setfill('0') << index;
            return key.str();
        }

        std::optional<std::string> optional_string(const DatSection &section, const char *key)
        {
            const auto field = section.find(key);
            return field == section.end() ? std::nullopt : std::optional<std::string>(field->second);
        }

        RiverGeometry parse_river(int id, const DatSection &section)
        {
            RiverGeometry river;
            river.id = id;
            river.name = optional_string(section, "name");
            river.type = optional_string(section, "type");
            river.texture = optional_string(section, "texture");
            river.multitexture = optional_string(section, "multitexture");
            river.flow = parse_short(section, "flow", 0);
            river.height = parse_int(section, "height", -1);
            river.color = parse_int(section, "color", 0);
            river.extend_pos_x = parse_int(section, "extend_posx", 0);
            river.extend_pos_y = parse_int(section, "extend_posy", 0);
            river.extend_neg_x = parse_int(section, "extend_negx", 0);
            river.extend_neg_y = parse_int(section, "extend_negy", 0);
            river.tessellation = parse_short(section, "tesselation", 0);

            const auto bank_count = parse_short(section, "bankpoints", 0);
            for (int index = 0; index < bank_count; ++index)
            {
                const auto left = section.find(numbered_key("left", index));
                const auto right = section.find(numbered_key("right", index));
                if (left == section.end() || right == section.end())
                {
                    continue;
                }
                river.banks.push_back({parse_bank_side(left->second), parse_bank_side(right->second)});
            }
            return river;
        }
    }

    std::vector<RiverGeometry> parse_rivers(const DatDocument &sector_data)
    {
        const auto definitions = sector_data.find("waterdefs");
        if (definitions == sector_data.end())
        {
            return {};
        }

        const auto count = parse_short(definitions->second, "num", 0);
        std::vector<RiverGeometry> rivers;
        for (int id = 0; id < count; ++id)
        {
            const auto section = sector_data.find(numbered_key("river", id));
            if (section != sector_data.end())
            {
                rivers.push_back(parse_river(id, section->second));
            }
        }
        return rivers;
    }
}