#include "daoc_client_lib/zone_geometry.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace
{
    void check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    std::filesystem::path make_client_root()
    {
        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto root = std::filesystem::temp_directory_path() / ("daoc-zone-test-" + std::to_string(unique));
        std::filesystem::create_directories(root);
        std::ofstream(root / "camelot.exe").put('\0');
        std::ofstream(root / "game.dll").put('\0');
        return root;
    }

    void test_zone_type_and_missing_sector_behavior()
    {
        const auto root = make_client_root();
        const daoc_client_lib::ClientDataDirectory client(root);
        const daoc_client_lib::ZoneGeometry dungeon(7, client, daoc_client_lib::ZoneType::dungeon);
        check(dungeon.id() == 7 && dungeon.is_dungeon(), "dungeon zone classification mismatch");
        check(!dungeon.is_city() && !dungeon.is_terrain(), "dungeon zone flags mismatch");
        check(dungeon.terrain_scale() == -1 && dungeon.size_x() == -1, "missing sector defaults mismatch");

        bool terrain_failed = false;
        try
        {
            daoc_client_lib::ZoneGeometry terrain(7, client, daoc_client_lib::ZoneType::terrain);
        }
        catch (const std::runtime_error&)
        {
            terrain_failed = true;
        }
        check(terrain_failed, "terrain zone must require sector.dat");
        std::filesystem::remove_all(root);
    }
}

int run_zone_geometry_tests()
{
    test_zone_type_and_missing_sector_behavior();
    return 0;
}