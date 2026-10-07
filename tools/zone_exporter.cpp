#include "daoc_client_lib/client_data_directory.h"
#include "daoc_client_lib/tree_replacement_map.h"
#include "daoc_client_lib/zone_data.h"
#include "daoc_client_lib/zone_geometry.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    const char* zone_type_name(daoc_client_lib::ZoneType type)
    {
        switch (type)
        {
        case daoc_client_lib::ZoneType::terrain: return "terrain";
        case daoc_client_lib::ZoneType::city: return "city";
        case daoc_client_lib::ZoneType::dungeon: return "dungeon";
        case daoc_client_lib::ZoneType::sky_city: return "skycity";
        default: return "unknown";
        }
    }

    void write_u32(std::ostream& output, std::uint32_t value)
    {
        const char bytes[] = {
            static_cast<char>(value),
            static_cast<char>(value >> 8),
            static_cast<char>(value >> 16),
            static_cast<char>(value >> 24)};
        output.write(bytes, sizeof(bytes));
    }

    void write_f32(std::ostream& output, float value)
    {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        write_u32(output, bits);
    }

    void write_height_map(
        const std::filesystem::path& path,
        const daoc_client_lib::TerrainHeightMap& height_map)
    {
        std::ofstream output(path, std::ios::binary);
        if (!output)
        {
            throw std::runtime_error("Could not create heightmap file: " + path.string());
        }
        output.write("DHT1", 4);
        write_u32(output, static_cast<std::uint32_t>(height_map.width()));
        write_u32(output, static_cast<std::uint32_t>(height_map.height()));
        for (std::size_t y = 0; y < height_map.height(); ++y)
        {
            for (std::size_t x = 0; x < height_map.width(); ++x)
            {
                write_f32(output, height_map.at(x, y));
            }
        }
        if (!output)
        {
            throw std::runtime_error("Failed while writing heightmap file: " + path.string());
        }
    }

    void write_placement(
        std::ostream& output,
        const daoc_client_lib::NifGeometry& geometry,
        const char* kind,
        float offset_x = 0.0f,
        float offset_y = 0.0f,
        float offset_z = 0.0f)
    {
        output << geometry.file_name << '\t'
            << std::setprecision(9) << geometry.x << '\t'
            << geometry.y << '\t'
            << geometry.z << '\t'
            << geometry.scale << '\t'
            << geometry.angle << '\t'
            << geometry.rotation_x << '\t'
            << geometry.rotation_y << '\t'
            << geometry.rotation_z << '\t'
            << (geometry.flip ? 1 : 0) << '\t'
            << (geometry.on_ground ? 1 : 0) << '\t'
            << kind << '\t'
            << offset_x << '\t'
            << offset_y << '\t'
            << offset_z << '\n';
    }

    void write_zone_export(
        const std::filesystem::path& output_directory,
        const daoc_client_lib::ZoneGeometry& zone,
        const daoc_client_lib::TreeReplacementMap& tree_replacements)
    {
        std::filesystem::create_directories(output_directory);
        std::set<std::string> assets;
        std::ofstream placements(output_directory / "placements.tsv", std::ios::binary);
        if (!placements)
        {
            throw std::runtime_error("Could not create placements.tsv");
        }
        placements << "NifName\tX\tY\tZ\tScale\tAngle\tRotationX\tRotationY\tRotationZ\tFlip\tOnGround\tKind\tOffsetX\tOffsetY\tOffsetZ\n";

        if (zone.is_terrain())
        {
            write_height_map(output_directory / "heightmap.dht", zone.terrain_height_map());
            for (const auto& fixture : zone.terrain_fixtures())
            {
                const auto replacements = tree_replacements.find(fixture.file_name);
                if (replacements.empty())
                {
                    write_placement(placements, fixture, "terrain_fixture");
                    assets.insert(fixture.file_name);
                    continue;
                }

                for (const auto& replacement : replacements)
                {
                    auto instance = fixture;
                    if (!replacement.real_nif.empty())
                    {
                        instance.file_name = replacement.real_nif;
                    }
                    write_placement(placements, instance, "terrain_fixture",
                        replacement.offset_x, replacement.offset_y, replacement.offset_z);
                    assets.insert(instance.file_name);
                }
            }
        }
        else if (zone.is_dungeon())
        {
            for (const auto& place : zone.dungeon_places())
            {
                write_placement(placements, place, "dungeon_place");
                assets.insert(place.file_name);
            }
            for (const auto& prop : zone.dungeon_props())
            {
                write_placement(placements, prop, "dungeon_prop");
                assets.insert(prop.file_name);
            }
        }
        else if (zone.is_city())
        {
            for (const auto& [id, nif_name] : zone.city_nifs())
            {
                static_cast<void>(id);
                daoc_client_lib::NifGeometry instance;
                instance.file_name = nif_name;
                write_placement(placements, instance, "city_mesh");
                assets.insert(nif_name);
            }
        }

        placements.close();
        if (!placements)
        {
            throw std::runtime_error("Failed while writing placements.tsv");
        }

        std::ofstream asset_file(output_directory / "assets.txt", std::ios::binary);
        if (!asset_file)
        {
            throw std::runtime_error("Could not create assets.txt");
        }
        for (const auto& asset : assets)
        {
            asset_file << asset << '\n';
        }

        std::ofstream metadata(output_directory / "zone.txt", std::ios::binary);
        if (!metadata)
        {
            throw std::runtime_error("Could not create zone.txt");
        }
        metadata << "ZoneId=" << zone.id() << '\n'
            << "ZoneType=" << zone_type_name(zone.type()) << '\n'
            << "TerrainScale=" << zone.terrain_scale() << '\n'
            << "TerrainOffset=" << zone.terrain_offset() << '\n'
            << "AssetCount=" << assets.size() << '\n';
    }
}

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: daoc_zone_exporter <client-root> <zone-id> <output-directory>\n";
        return 2;
    }

    try
    {
        const int zone_id = std::stoi(argv[2]);
        const daoc_client_lib::ClientDataDirectory client(argv[1]);
        const auto zones = daoc_client_lib::read_zones(client);
        const auto zone_data = std::find_if(zones.begin(), zones.end(), [zone_id](const auto& zone)
        {
            return zone.id == zone_id;
        });
        if (zone_data == zones.end())
        {
            throw std::runtime_error("Zone ID is not present in zones.dat");
        }

        // Missing type denotes ordinary terrain (e.g. Dartmoor and Atlantis).
        // Housing type 3 shares the same height/fixture representation.
        const auto zone_type = static_cast<daoc_client_lib::ZoneType>(
            zone_data->type < 0 || zone_data->type == 3 ? 0 : zone_data->type);
        const daoc_client_lib::ZoneGeometry zone(zone_id, client, zone_type);
        const auto tree_replacements = daoc_client_lib::TreeReplacementMap::load(client);
        write_zone_export(argv[3], zone, tree_replacements);
        std::cout << "Exported zone " << zone_id << " to " << argv[3] << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Zone export failed: " << error.what() << '\n';
        return 1;
    }
}
