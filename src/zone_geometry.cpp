#include "daoc_client_lib/zone_geometry.h"

#include "daoc_client_lib/text_data.h"

#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace daoc_client_lib
{
    namespace
    {
        std::string zone_package_name(int id)
        {
            std::ostringstream name;
            name << "dat" << std::setw(3) << std::setfill('0') << id << ".mpk";
            return name.str();
        }

        std::int16_t parse_short(const DatSection &section, const char *key, std::int16_t fallback)
        {
            const auto field = section.find(key);
            if (field == section.end())
            {
                return fallback;
            }

            int value = 0;
            const auto result = std::from_chars(
                field->second.data(), field->second.data() + field->second.size(), value);
            if (result.ec != std::errc{} || result.ptr != field->second.data() + field->second.size() || value < std::numeric_limits<std::int16_t>::min() || value > std::numeric_limits<std::int16_t>::max())
            {
                throw std::runtime_error("Invalid sector.dat integer field: " + std::string(key));
            }
            return static_cast<std::int16_t>(value);
        }

        std::string bytes_to_string(const std::vector<std::uint8_t> &bytes)
        {
            return {bytes.begin(), bytes.end()};
        }
    }

    ZoneGeometry::ZoneGeometry(int id, const ClientDataDirectory &client, ZoneType type)
        : id_(id), client_(client), type_(type)
    {
        DatDocument sector;
        try
        {
            sector = parse_dat(bytes_to_string(read_zone_file("sector.dat")));
        }
        catch (const std::exception &error)
        {
            if (type_ == ZoneType::terrain)
            {
                throw std::runtime_error("No usable sector.dat for zone " + std::to_string(id_) + ": " + error.what());
            }
        }

        const auto terrain = sector.find("terrain");
        if (terrain != sector.end())
        {
            terrain_scale_ = parse_short(terrain->second, "scalefactor", -1);
            terrain_offset_ = parse_short(terrain->second, "offsetfactor", -1);
        }
        const auto sector_size = sector.find("sectorsize");
        if (sector_size != sector.end())
        {
            size_x_ = parse_short(sector_size->second, "sizex", -1);
            size_y_ = parse_short(sector_size->second, "sizey", -1);
        }
        rivers_ = parse_rivers(sector);
    }

    int ZoneGeometry::id() const noexcept
    {
        return id_;
    }

    ZoneType ZoneGeometry::type() const noexcept
    {
        return type_;
    }

    std::int16_t ZoneGeometry::terrain_scale() const noexcept
    {
        return terrain_scale_;
    }

    std::int16_t ZoneGeometry::terrain_offset() const noexcept
    {
        return terrain_offset_;
    }

    std::int16_t ZoneGeometry::size_x() const noexcept
    {
        return size_x_;
    }

    std::int16_t ZoneGeometry::size_y() const noexcept
    {
        return size_y_;
    }

    bool ZoneGeometry::is_terrain() const noexcept
    {
        return type_ == ZoneType::terrain;
    }

    bool ZoneGeometry::is_dungeon() const noexcept
    {
        return type_ == ZoneType::dungeon || type_ == ZoneType::sky_city;
    }

    bool ZoneGeometry::is_city() const noexcept
    {
        return type_ == ZoneType::city;
    }

    const std::vector<RiverGeometry> &ZoneGeometry::rivers() const noexcept
    {
        return rivers_;
    }

    std::map<std::int16_t, ZoneJump> ZoneGeometry::zone_jumps() const
    {
        return parse_zone_jumps(parse_csv(bytes_to_string(read_zone_file("zonejump.csv"))));
    }

    ZoneBounds ZoneGeometry::bounds() const
    {
        return parse_zone_bounds(parse_csv(bytes_to_string(read_zone_file("bound.csv"))));
    }

    std::map<int, std::string> ZoneGeometry::terrain_nifs() const
    {
        return parse_terrain_nifs(parse_csv(bytes_to_string(read_zone_file("nifs.csv"))));
    }

    std::vector<NifGeometry> ZoneGeometry::terrain_fixtures() const
    {
        const auto nifs = terrain_nifs();
        const auto rows = parse_csv(bytes_to_string(read_zone_file("fixtures.csv")));
        return parse_terrain_fixtures(rows, nifs);
    }

    std::vector<std::string> ZoneGeometry::dungeon_chunks() const
    {
        const auto chunk_file = type_ == ZoneType::sky_city ? "skycity.chunk" : "dungeon.chunk";
        const auto chunk_rows = parse_csv(bytes_to_string(read_zone_file(chunk_file)));
        CsvTable proxies;
        if (const auto bytes = client_.try_read_package_file(zone_package_name(id_), "nifproxy.csv"))
        {
            proxies = parse_csv(bytes_to_string(*bytes));
        }
        return parse_dungeon_chunks(chunk_rows, proxies);
    }

    std::vector<NifGeometry> ZoneGeometry::dungeon_places() const
    {
        const auto chunks = dungeon_chunks();
        const auto place_file = type_ == ZoneType::sky_city ? "skycity.place" : "dungeon.place";
        return parse_dungeon_places(parse_csv(bytes_to_string(read_zone_file(place_file))), chunks);
    }

    std::vector<NifGeometry> ZoneGeometry::dungeon_props() const
    {
        const auto chunks = dungeon_chunks();
        const auto places = dungeon_places();
        const auto prop_file = type_ == ZoneType::sky_city ? "skycity.prop" : "dungeon.prop";
        return parse_dungeon_props(parse_csv(bytes_to_string(read_zone_file(prop_file))), chunks, places);
    }

    std::map<int, DungeonWaterGeometry> ZoneGeometry::dungeon_water_definitions() const
    {
        const auto filename = type_ == ZoneType::sky_city ? "skycity.water" : "dungeon.water";
        return parse_dungeon_water_definitions(parse_csv(bytes_to_string(read_zone_file(filename))));
    }

    std::map<int, int> ZoneGeometry::dungeon_water_locations() const
    {
        const auto filename = type_ == ZoneType::sky_city ? "skycity.water.csv" : "dungeon.water.csv";
        return parse_dungeon_water_locations(parse_csv(bytes_to_string(read_zone_file(filename))));
    }

    std::map<int, std::string> ZoneGeometry::city_nifs() const
    {
        return parse_city_nifs(parse_csv(bytes_to_string(read_zone_file("city.csv"))));
    }

    std::vector<std::uint8_t> ZoneGeometry::read_zone_file(const char *filename) const
    {
        return client_.read_package_file(zone_package_name(id_), filename);
    }

    TerrainHeightMap ZoneGeometry::terrain_height_map() const
    {
        const auto terrain = PcxImage::decode(read_zone_file("terrain.pcx"));
        const auto offset = PcxImage::decode(read_zone_file("offset.pcx"));
        return TerrainHeightMap(terrain, offset, terrain_scale_, terrain_offset_);
    }

    std::vector<std::uint8_t> ZoneGeometry::water_index_map() const
    {
        const auto water = PcxImage::decode(read_zone_file("water.pcx"));
        std::vector<std::uint8_t> indices;
        indices.reserve(water.width() * water.height());
        for (std::size_t y = 0; y < water.height(); ++y)
        {
            for (std::size_t x = 0; x < water.width(); ++x)
            {
                indices.push_back(water.pixel(x, y).red);
            }
        }
        return indices;
    }

    std::size_t ZoneGeometry::map_width() const
    {
        return PcxImage::decode(read_zone_file("terrain.pcx")).width();
    }

    std::size_t ZoneGeometry::map_height() const
    {
        return PcxImage::decode(read_zone_file("terrain.pcx")).height();
    }
}