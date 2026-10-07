#pragma once

#include "daoc_client_lib/text_data.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace daoc_client_lib
{
    struct NifGeometry
    {
        int mesh_id = 0;
        int id = 0;
        std::string file_name;
        std::string textual_name;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float scale = 1.0f;
        float angle = 0.0f;
        float rotation_x = 0.0f;
        float rotation_y = 0.0f;
        float rotation_z = 0.0f;
        bool flip = false;
        bool on_ground = false;
        std::optional<int> relative_to_id;
    };

    std::map<int, std::string> parse_terrain_nifs(const CsvTable& rows);
    std::map<int, std::string> parse_city_nifs(const CsvTable& rows);
    std::vector<NifGeometry> parse_terrain_fixtures(
        const CsvTable& rows,
        const std::map<int, std::string>& terrain_nifs);
    std::vector<std::string> parse_dungeon_chunks(const CsvTable& rows, const CsvTable& proxies = {});
    std::vector<NifGeometry> parse_dungeon_places(
        const CsvTable& rows,
        const std::vector<std::string>& chunks);
    std::vector<NifGeometry> parse_dungeon_props(
        const CsvTable& rows,
        const std::vector<std::string>& chunks,
        const std::vector<NifGeometry>& places);
}