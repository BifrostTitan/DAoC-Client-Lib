#include "daoc_client_lib/river_geometry.h"

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

    void test_river_sections_and_banks()
    {
        const auto sector = daoc_client_lib::parse_dat(
            "[waterdefs]\nnum=2\n"
            "[river00]\nname=riverA\ntype=RIVER\ntexture=water.dds\nmultitexture=foam.dds\n"
            "flow=3\nheight=12\ncolor=16777215\nextend_posx=1\nextend_posy=2\n"
            "extend_negx=3\nextend_negy=4\ntesselation=5\nbankpoints=1\n"
            "left00=1, 2, 3\nright00=4, 5, 6\n"
            "[river01]\nheight=20\n");
        const auto rivers = daoc_client_lib::parse_rivers(sector);
        check(rivers.size() == 2, "river count mismatch");
        check(rivers[0].id == 0 && rivers[0].name == "riverA" && rivers[0].type == "RIVER",
              "river identity mismatch");
        check(rivers[0].flow == 3 && rivers[0].height == 12 && rivers[0].color == 16777215,
              "river numeric fields mismatch");
        check(rivers[0].banks.size() == 1, "river bank count mismatch");
        check(rivers[0].banks[0].left[2] == 3 && rivers[0].banks[0].right[0] == 4,
              "river bank coordinates mismatch");
        check(rivers[1].height == 20 && rivers[1].flow == 0, "river defaults mismatch");
    }
}

int run_river_geometry_tests()
{
    test_river_sections_and_banks();
    return 0;
}