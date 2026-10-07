#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace daoc_client_lib
{
    using CsvTable = std::vector<std::vector<std::string>>;
    using DatSection = std::map<std::string, std::string>;
    using DatDocument = std::map<std::string, DatSection>;

    CsvTable parse_csv(std::string_view input);
    DatDocument parse_dat(std::string_view input);
}