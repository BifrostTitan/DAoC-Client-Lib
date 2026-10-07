#include "daoc_client_lib/zone_geometry_chooser.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    void check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void test_zone_folder_and_catalog_lookup()
    {
        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto root = std::filesystem::temp_directory_path() / ("daoc-chooser-test-" + std::to_string(unique));
        std::filesystem::create_directories(root / "zone1");
        std::ofstream(root / "camelot.exe").put('\0');
        std::ofstream(root / "game.dll").put('\0');
        std::ofstream(root / "zone1" / "map.pcx").put('\0');

        const daoc_client_lib::ClientDataDirectory client(root);
        daoc_client_lib::ZoneData city;
        city.id = 1;
        city.type = 1;
        const daoc_client_lib::ZoneGeometryChooser chooser(client, std::vector<daoc_client_lib::ZoneData>{city});
        const auto geometry = chooser.find(1);
        check(geometry.has_value() && geometry->is_city(), "zone chooser did not resolve city geometry");
        check(!chooser.find(2).has_value(), "zone chooser returned an unregistered zone");
        std::filesystem::remove_all(root);
    }
}

int run_zone_geometry_chooser_tests()
{
    test_zone_folder_and_catalog_lookup();
    return 0;
}