#include "daoc_client_lib/nif_geometry.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <utility>

namespace daoc_client_lib
{
    namespace
    {
        constexpr float pi = 3.14159265358979323846f;

        int parse_int(const std::string &value)
        {
            int parsed = 0;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
            {
                throw std::runtime_error("Invalid integer in NIF geometry CSV: " + value);
            }
            return parsed;
        }

        float parse_float(const std::string &value)
        {
            float parsed = 0.0f;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || !std::isfinite(parsed))
            {
                throw std::runtime_error("Invalid float in NIF geometry CSV: " + value);
            }
            return parsed;
        }

        bool starts_with_ascii_case_insensitive(const std::string &value, const char *prefix)
        {
            std::size_t index = 0;
            for (; prefix[index] != '\0' && index < value.size(); ++index)
            {
                auto character = static_cast<unsigned char>(value[index]);
                if (character >= 'A' && character <= 'Z')
                {
                    character = static_cast<unsigned char>(character + ('a' - 'A'));
                }
                auto expected = static_cast<unsigned char>(prefix[index]);
                if (expected >= 'A' && expected <= 'Z')
                {
                    expected = static_cast<unsigned char>(expected + ('a' - 'A'));
                }
                if (character != expected)
                {
                    return false;
                }
            }
            return prefix[index] == '\0';
        }

        bool equals_ascii_case_insensitive(const std::string &left, const std::string &right)
        {
            if (left.size() != right.size())
            {
                return false;
            }
            for (std::size_t index = 0; index < left.size(); ++index)
            {
                auto left_character = static_cast<unsigned char>(left[index]);
                auto right_character = static_cast<unsigned char>(right[index]);
                if (left_character >= 'A' && left_character <= 'Z')
                {
                    left_character = static_cast<unsigned char>(left_character + ('a' - 'A'));
                }
                if (right_character >= 'A' && right_character <= 'Z')
                {
                    right_character = static_cast<unsigned char>(right_character + ('a' - 'A'));
                }
                if (left_character != right_character)
                {
                    return false;
                }
            }
            return true;
        }

        std::string chunk_name(int mesh_id, const std::vector<std::string> &chunks)
        {
            if (mesh_id < 0 || static_cast<std::size_t>(mesh_id) >= chunks.size())
            {
                throw std::out_of_range("NIF geometry references a missing chunk");
            }
            return chunks[static_cast<std::size_t>(mesh_id)];
        }
    }

    std::map<int, std::string> parse_terrain_nifs(const CsvTable &rows)
    {
        std::map<int, std::string> nifs;
        for (const auto &row : rows)
        {
            if (row.size() < 3 || starts_with_ascii_case_insensitive(row[0], "Grid Nifs") || starts_with_ascii_case_insensitive(row[0], "NIF"))
            {
                continue;
            }
            nifs.emplace(parse_int(row[0]), row[2]);
        }
        return nifs;
    }

    std::map<int, std::string> parse_city_nifs(const CsvTable &rows)
    {
        std::map<int, std::string> nifs;
        for (const auto &row : rows)
        {
            if (row.size() < 2 || row[1].empty())
            {
                continue;
            }
            if (!nifs.emplace(parse_int(row[0]), row[1]).second)
            {
                throw std::runtime_error("Duplicate city NIF ID");
            }
        }
        return nifs;
    }

    std::vector<NifGeometry> parse_terrain_fixtures(
        const CsvTable &rows,
        const std::map<int, std::string> &terrain_nifs)
    {
        std::vector<NifGeometry> fixtures;
        int index = -1;
        for (const auto &row : rows)
        {
            if (row.size() < 13 || starts_with_ascii_case_insensitive(row[0], "Fixtures") || starts_with_ascii_case_insensitive(row[0], "ID"))
            {
                continue;
            }
            ++index;

            const int mesh_id = parse_int(row[1]);
            const auto nif = terrain_nifs.find(mesh_id);
            if (nif == terrain_nifs.end())
            {
                continue;
            }

            const int direction = parse_int(row[6]);
            NifGeometry fixture;
            fixture.mesh_id = mesh_id;
            fixture.id = index;
            fixture.file_name = nif->second;
            fixture.textual_name = row[2];
            fixture.x = parse_float(row[3]);
            fixture.y = parse_float(row[4]);
            fixture.z = parse_float(row[5]);
            fixture.scale = parse_int(row[7]) / 100.0f;
            fixture.on_ground = parse_int(row[11]) > 0;
            fixture.flip = parse_int(row[12]) != 0;
            if (direction > 180)
            {
                fixture.angle = (direction - 360) * pi / -180.0f;
                fixture.rotation_z = 1.0f;
            }
            else
            {
                fixture.angle = direction * pi / 180.0f;
                fixture.rotation_z = -1.0f;
            }
            if (row.size() > 18)
            {
                fixture.angle = parse_float(row[15]);
                fixture.rotation_x = parse_float(row[16]);
                fixture.rotation_y = parse_float(row[17]);
                fixture.rotation_z = parse_float(row[18]);
            }
            fixtures.push_back(std::move(fixture));
        }
        return fixtures;
    }

    std::vector<std::string> parse_dungeon_chunks(const CsvTable &rows, const CsvTable &proxies)
    {
        std::vector<std::string> chunks;
        for (const auto &row : rows)
        {
            if (row.empty())
            {
                continue;
            }
            auto name = row[0];
            if (name.empty())
            {
                chunks.emplace_back();
                continue;
            }
            const auto proxy = std::find_if(proxies.begin(), proxies.end(), [&name](const auto &candidate)
                                            { return candidate.size() > 1 && equals_ascii_case_insensitive(name, candidate[0]); });
            if (proxy != proxies.end() && !proxy->at(1).empty())
            {
                name = proxy->at(1);
            }
            chunks.push_back(std::move(name));
        }
        return chunks;
    }

    std::vector<NifGeometry> parse_dungeon_places(
        const CsvTable &rows,
        const std::vector<std::string> &chunks)
    {
        std::vector<NifGeometry> places;
        int index = -1;
        for (const auto &row : rows)
        {
            if (row.size() < 8)
            {
                continue;
            }
            ++index;
            const auto mesh_id = parse_int(row[0]);
            NifGeometry place;
            place.mesh_id = mesh_id;
            place.id = index;
            place.file_name = chunk_name(mesh_id, chunks);
            place.textual_name = place.file_name;
            place.x = parse_float(row[1]);
            place.y = parse_float(row[2]);
            place.z = parse_float(row[3]);
            place.angle = parse_float(row[4]);
            place.rotation_x = parse_float(row[5]);
            place.rotation_y = parse_float(row[6]);
            place.rotation_z = parse_float(row[7]);
            if (row.size() > 11)
            {
                place.flip = parse_int(row[11]) != 0;
            }
            places.push_back(std::move(place));
        }
        return places;
    }

    std::vector<NifGeometry> parse_dungeon_props(
        const CsvTable &rows,
        const std::vector<std::string> &chunks,
        const std::vector<NifGeometry> &places)
    {
        std::vector<NifGeometry> props;
        int index = -1;
        for (const auto &row : rows)
        {
            if (row.size() < 11)
            {
                continue;
            }
            ++index;
            const auto mesh_id = parse_int(row[0]);
            const auto place_id = parse_int(row[9]);
            NifGeometry prop;
            prop.mesh_id = mesh_id;
            prop.id = index;
            prop.file_name = chunk_name(mesh_id, chunks);
            prop.textual_name = prop.file_name;
            prop.x = parse_float(row[1]);
            prop.y = parse_float(row[2]);
            prop.z = parse_float(row[3]);
            prop.angle = parse_float(row[4]);
            prop.rotation_x = parse_float(row[5]);
            prop.rotation_y = parse_float(row[6]);
            prop.rotation_z = parse_float(row[7]);
            prop.scale = parse_float(row[10]);
            const auto place = std::find_if(places.begin(), places.end(), [place_id](const auto &candidate)
                                            { return candidate.id == place_id; });
            if (place != places.end())
            {
                prop.relative_to_id = place->id;
            }
            props.push_back(std::move(prop));
        }
        return props;
    }
}