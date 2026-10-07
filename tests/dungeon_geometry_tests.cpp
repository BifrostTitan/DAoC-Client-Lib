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

    void test_dungeon_placements_and_proxy_names()
    {
        const auto chunks = daoc_client_lib::parse_dungeon_chunks(
            daoc_client_lib::parse_csv("stone.nif\nprop.nif\n"),
            daoc_client_lib::parse_csv("STONE.NIF,stone_alt.nif\n"));
        check(chunks.size() == 2 && chunks[0] == "stone_alt.nif", "dungeon proxy replacement mismatch");

        const auto places = daoc_client_lib::parse_dungeon_places(
            daoc_client_lib::parse_csv("0,1,2,3,0.5,0,0,1,unused,unused,unused,1\n"), chunks);
        check(places.size() == 1 && places[0].id == 0 && places[0].flip,
              "dungeon place record mismatch");

        const auto props = daoc_client_lib::parse_dungeon_props(
            daoc_client_lib::parse_csv("1,4,5,6,0,0,1,0,unused,0,0.75\n"), chunks, places);
        check(props.size() == 1 && props[0].relative_to_id == 0, "dungeon prop parent mismatch");
        check(props[0].file_name == "prop.nif" && props[0].scale == 0.75f,
              "dungeon prop transform mismatch");
    }
}

int run_dungeon_geometry_tests()
{
    test_dungeon_placements_and_proxy_names();
    return 0;
}