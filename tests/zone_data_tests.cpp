#include "daoc_client_lib/zone_data.h"

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

    void test_zone_fields_and_defaults()
    {
        const auto document = daoc_client_lib::parse_dat(
            "[zone42]\n"
            "enabled=1\n"
            "type=2\n"
            "region=5\n"
            "name=Dark Zone\n"
            "region_offset_x=10\n"
            "region_offset_y=20\n"
            "width=64\n"
            "height=32\n"
            "temperature=-8\n"
            "entry_music=12\n"
            "skydome=sky01\n"
            "map_enabled=0\n"
            "proxy_zone=7\n"
            "[other42]\n"
            "enabled=1\n");
        const auto zones = daoc_client_lib::parse_zones(document);
        check(zones.size() == 1, "zone header filtering mismatch");
        const auto& zone = zones.front();
        check(zone.id == 42 && zone.type == 2 && zone.region == 5, "zone identifiers mismatch");
        check(zone.enabled && zone.name == "Dark Zone", "zone enabled/name mismatch");
        check(zone.offset_x == 10 && zone.offset_y == 20 && zone.width == 64 && zone.height == 32,
              "zone dimensions mismatch");
        check(zone.temperature == -8 && zone.entry_music == 12, "zone optional numeric fields mismatch");
        check(zone.sky_dome == "sky01" && !zone.map_enabled, "zone display fields mismatch");
        check(zone.proxy_zone == 7 && zone.is_proxy_zone(), "zone proxy fields mismatch");
    }

    void test_zone_defaults()
    {
        const auto zones = daoc_client_lib::parse_zones(daoc_client_lib::parse_dat("[zone1]\n"));
        check(zones.size() == 1, "default zone parse failed");
        check(!zones[0].enabled && zones[0].type == -1 && zones[0].region == -1, "zone defaults mismatch");
        check(zones[0].temperature == INT16_MIN && zones[0].map_enabled, "zone default flags mismatch");
    }
}

int run_zone_data_tests()
{
    test_zone_fields_and_defaults();
    test_zone_defaults();
    return 0;
}