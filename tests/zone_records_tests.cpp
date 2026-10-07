#include "daoc_client_lib/zone_records.h"

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

    void test_zone_jumps()
    {
        const auto rows = daoc_client_lib::parse_csv("0,North Gate,1,2,3,4,5,6,7\nshort,row\n");
        const auto jumps = daoc_client_lib::parse_zone_jumps(rows);
        check(jumps.size() == 1, "zone jump filtering mismatch");
        check(jumps.at(7).order == 0 && jumps.at(7).name == "North Gate", "zone jump identity mismatch");
        check(jumps.at(7).x2 == 3 && jumps.at(7).z2 == 6, "zone jump geometry mismatch");
    }

    void test_zone_bounds()
    {
        const auto bounds = daoc_client_lib::parse_zone_bounds(
            daoc_client_lib::parse_csv("area,2,10,20,30,40\npartial,2,1,2,3\n"));
        check(bounds.size() == 2, "zone bounds row count mismatch");
        check(bounds[0].size() == 2 && bounds[0][1].x == 30 && bounds[0][1].y == 40,
              "zone boundary points mismatch");
        check(bounds[1].size() == 1, "incomplete boundary coordinate should be ignored");
    }
}

int run_zone_records_tests()
{
    test_zone_jumps();
    test_zone_bounds();
    return 0;
}