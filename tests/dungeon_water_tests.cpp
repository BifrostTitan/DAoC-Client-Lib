#include "daoc_client_lib/dungeon_water.h"
#include "daoc_client_lib/nif_geometry.h"

#include <stdexcept>

namespace
{
    void check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void test_water_definitions_and_locations()
    {
        const auto definitions = daoc_client_lib::parse_dungeon_water_definitions(
            daoc_client_lib::parse_csv("4,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15\nshort,row\n"));
        check(definitions.size() == 1, "dungeon water row filtering mismatch");
        check(definitions.at(4).corners[0].x == 1.0f && definitions.at(4).corners[3].z == 12.0f,
              "dungeon water corner parsing mismatch");
        check(definitions.at(4).translation.x == 13.0f && definitions.at(4).translation.z == 15.0f,
              "dungeon water translation mismatch");

        const auto locations = daoc_client_lib::parse_dungeon_water_locations(
            daoc_client_lib::parse_csv("3,4\nshort\n"));
        check(locations.size() == 1 && locations.at(3) == 4, "dungeon water location mismatch");

        const auto city_nifs = daoc_client_lib::parse_city_nifs(
            daoc_client_lib::parse_csv("1,city.nif\n2,\n"));
        check(city_nifs.size() == 1 && city_nifs.at(1) == "city.nif", "city NIF index mismatch");
    }
}

int run_dungeon_water_tests()
{
    test_water_definitions_and_locations();
    return 0;
}