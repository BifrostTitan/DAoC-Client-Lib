#include "daoc_client_lib/text_data.h"

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

    void test_csv()
    {
        const auto rows = daoc_client_lib::parse_csv(
            "Header, Col, Some Tests, \"quoted\",19,87\n0, 1, 2, 3, 4, 5, 6, 7\n");
        check(rows.size() == 2, "CSV row count mismatch");
        check(rows[0].size() == 6 && rows[0][1] == "Col", "CSV field trimming mismatch");
        check(rows[0][3] == "\"quoted\"", "CSV quotes should remain literal");
        check(rows[1].size() == 8 && rows[1][7] == "7", "CSV second row mismatch");
    }

    void test_dat()
    {
        const auto document = daoc_client_lib::parse_dat(
            ";comment ... ...\n"
            ";some more comments ...\n"
            "; comments ; in ; comments ; !\n"
            "[region0];come comments ; with comments\n"
            "[region1]\n"
            "Var=Value ; comment; with comments\n"
            "Var2 = Value2[OtherRegion0]\n"
            "OtherVar=OtherValue\n"
            "OtherVar2 = OtherValue2\n"
            "[OtherRegion1]\n"
            "OtherVar= OtherParam\n"
            "OtherVar2 =OtherParam2");
        check(document.size() == 4, "DAT region count mismatch");
        check(document.at("region0").empty(), "empty DAT region was not preserved");
        check(document.at("region1").at("var") == "Value", "DAT comment handling mismatch");
        check(document.at("region1").at("var2") == "Value2", "DAT adjacent region parsing mismatch");
        check(document.at("otherregion0").at("othervar2") == "OtherValue2", "DAT key/value parse mismatch");
    }
}

int run_text_data_tests()
{
    test_csv();
    test_dat();
    return 0;
}