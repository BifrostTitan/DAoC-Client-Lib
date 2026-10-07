#include "daoc_client_lib/text_data.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace daoc_client_lib
{
    namespace
    {
        std::string trim(std::string_view value)
        {
            const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char character)
                                                { return std::isspace(character) != 0; });
            const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char character)
                                               { return std::isspace(character) != 0; })
                                  .base();
            if (first >= last)
            {
                return {};
            }
            return std::string(first, last);
        }

        std::string lower_ascii(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
                           { return static_cast<char>(std::tolower(character)); });
            return value;
        }

        bool allowed_dat_character(unsigned char character)
        {
            return std::isalnum(character) != 0 || character == '\'' || character == '_' || character == '-' || character == '.' || character == ',' || character == ' ';
        }

        std::string without_comments(std::string_view input)
        {
            std::string output;
            output.reserve(input.size());
            bool in_comment = false;
            for (const char character : input)
            {
                if (character == '\n')
                {
                    in_comment = false;
                    output.push_back(character);
                }
                else if (character == ';')
                {
                    in_comment = true;
                }
                else if (!in_comment)
                {
                    output.push_back(character);
                }
            }
            return output;
        }

        DatSection parse_section(std::string_view content)
        {
            DatSection section;
            std::size_t line_begin = 0;
            while (line_begin <= content.size())
            {
                const auto line_end = content.find('\n', line_begin);
                const auto line = content.substr(line_begin,
                                                 line_end == std::string_view::npos ? content.size() - line_begin : line_end - line_begin);
                const auto equals = line.find('=');
                if (equals != std::string_view::npos)
                {
                    const auto key = trim(line.substr(0, equals));
                    const auto value_source = line.substr(equals + 1);
                    std::size_t value_end = 0;
                    while (value_end < value_source.size() && allowed_dat_character(static_cast<unsigned char>(value_source[value_end])))
                    {
                        ++value_end;
                    }
                    const auto value = trim(value_source.substr(0, value_end));
                    if (!key.empty() && std::all_of(key.begin(), key.end(), [](unsigned char character)
                                                    { return allowed_dat_character(character); }))
                    {
                        section.emplace(lower_ascii(key), value);
                    }
                }
                if (line_end == std::string_view::npos)
                {
                    break;
                }
                line_begin = line_end + 1;
            }
            return section;
        }
    }

    CsvTable parse_csv(std::string_view input)
    {
        CsvTable table;
        std::size_t line_begin = 0;
        while (line_begin < input.size())
        {
            const auto line_end = input.find('\n', line_begin);
            const auto line = trim(input.substr(line_begin,
                                                line_end == std::string_view::npos ? input.size() - line_begin : line_end - line_begin));
            if (!line.empty())
            {
                std::vector<std::string> row;
                std::size_t field_begin = 0;
                while (true)
                {
                    const auto comma = line.find(',', field_begin);
                    row.push_back(trim(std::string_view(line).substr(field_begin,
                                                                     comma == std::string::npos ? line.size() - field_begin : comma - field_begin)));
                    if (comma == std::string::npos)
                    {
                        break;
                    }
                    field_begin = comma + 1;
                }
                table.push_back(std::move(row));
            }
            if (line_end == std::string_view::npos)
            {
                break;
            }
            line_begin = line_end + 1;
        }
        return table;
    }

    DatDocument parse_dat(std::string_view input)
    {
        const auto content = without_comments(input);
        DatDocument document;
        std::size_t search_from = 0;
        while (search_from < content.size())
        {
            const auto open = content.find('[', search_from);
            if (open == std::string::npos)
            {
                break;
            }
            const auto close = content.find(']', open + 1);
            if (close == std::string::npos)
            {
                break;
            }
            const auto next_open = content.find('[', close + 1);
            const auto section_end = next_open == std::string::npos ? content.size() : next_open;
            const auto name = lower_ascii(trim(std::string_view(content).substr(open + 1, close - open - 1)));
            if (!name.empty() && section_end > close + 1)
            {
                document.emplace(name, parse_section(std::string_view(content).substr(
                                           close + 1, section_end - close - 1)));
            }
            search_from = section_end;
        }
        return document;
    }
}