#include "daoc_client_lib/nif_geometry.h"

#include <cmath>
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

    void test_terrain_fixture_transform()
    {
        const auto nifs = daoc_client_lib::parse_terrain_nifs(
            daoc_client_lib::parse_csv("NIF ID,Unused,File\n3,x,tree.nif\n"));
        check(nifs.size() == 1 && nifs.at(3) == "tree.nif", "terrain NIF index mismatch");

        const auto rows = daoc_client_lib::parse_csv(
            "Fixtures,Header,Name,A,B,C,D,E,F,G,H,I,J\n"
            "0,3,Oak,1.5,2.5,3.5,270,125,x,x,0,1,1\n"
            "1,99,Missing,0,0,0,0,100,x,x,0,0,0\n");
        const auto fixtures = daoc_client_lib::parse_terrain_fixtures(rows, nifs);
        check(fixtures.size() == 1, "unmapped terrain fixture should be skipped");
        const auto& fixture = fixtures.front();
        check(fixture.mesh_id == 3 && fixture.id == 0 && fixture.file_name == "tree.nif",
              "terrain fixture identity mismatch");
        check(fixture.textual_name == "Oak" && fixture.x == 1.5f && fixture.scale == 1.25f,
              "terrain fixture transform mismatch");
        check(fixture.on_ground && fixture.flip && fixture.rotation_z == 1.0f,
              "terrain fixture flags mismatch");
        check(std::fabs(fixture.angle - 1.57079632679f) < 0.0001f,
              "terrain fixture angle conversion mismatch");
    }
}

int run_nif_geometry_tests()
{
    test_terrain_fixture_transform();
    return 0;
}