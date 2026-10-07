#pragma once

#include "daoc_client_lib/client_data_directory.h"
#include "daoc_client_lib/dungeon_water.h"
#include "daoc_client_lib/nif_geometry.h"
#include "daoc_client_lib/river_geometry.h"
#include "daoc_client_lib/terrain_height_map.h"
#include "daoc_client_lib/zone_records.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace daoc_client_lib
{
    enum class ZoneType : std::int16_t
    {
        terrain = 0,
        city = 1,
        dungeon = 2,
        sky_city = 4
    };

    class ZoneGeometry
    {
    public:
        ZoneGeometry(int id, const ClientDataDirectory& client, ZoneType type);

        int id() const noexcept;
        ZoneType type() const noexcept;
        std::int16_t terrain_scale() const noexcept;
        std::int16_t terrain_offset() const noexcept;
        std::int16_t size_x() const noexcept;
        std::int16_t size_y() const noexcept;
        bool is_terrain() const noexcept;
        bool is_dungeon() const noexcept;
        bool is_city() const noexcept;
        const std::vector<RiverGeometry>& rivers() const noexcept;
        std::map<std::int16_t, ZoneJump> zone_jumps() const;
        std::vector<std::vector<ZonePoint>> bounds() const;
        std::map<int, std::string> terrain_nifs() const;
        std::vector<NifGeometry> terrain_fixtures() const;
        std::vector<std::string> dungeon_chunks() const;
        std::vector<NifGeometry> dungeon_places() const;
        std::vector<NifGeometry> dungeon_props() const;
        std::map<int, DungeonWaterGeometry> dungeon_water_definitions() const;
        std::map<int, int> dungeon_water_locations() const;
        std::map<int, std::string> city_nifs() const;

        TerrainHeightMap terrain_height_map() const;
        std::vector<std::uint8_t> water_index_map() const;
        std::size_t map_width() const;
        std::size_t map_height() const;

    private:
        int id_;
        ClientDataDirectory client_;
        ZoneType type_;
        std::int16_t terrain_scale_ = -1;
        std::int16_t terrain_offset_ = -1;
        std::int16_t size_x_ = -1;
        std::int16_t size_y_ = -1;
        std::vector<RiverGeometry> rivers_;

        std::vector<std::uint8_t> read_zone_file(const char* filename) const;
    };
}