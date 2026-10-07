#include "daoc_client_lib/jump_point.h"

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

    void test_jump_point_csv()
    {
        const auto points = daoc_client_lib::parse_jump_points(
            daoc_client_lib::parse_csv("1,Capital,10,100,200,300,512,11,0,invalid,12\n"));
        check(points.size() == 1, "jump point row count mismatch");
        check(points[0].id == 1 && points[0].name == "Capital" && points[0].zone_id == 10,
              "jump point identity mismatch");
        check(points[0].heading_degrees == 512 && points[0].x == 100 && points[0].z == 300,
              "jump point position mismatch");
        check(points[0].also_in_zone_ids.size() == 2 && points[0].also_in_zone_ids[1] == 12,
              "jump point mirror zones mismatch");
    }
}

int run_jump_point_tests()
{
    test_jump_point_csv();
    return 0;
}