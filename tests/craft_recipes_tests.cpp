#include "daoc_client_lib/craft_recipes.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    void check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void write_u16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value)
    {
        bytes[offset] = static_cast<std::uint8_t>(value);
        bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8);
    }

    void write_u32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value)
    {
        for (int index = 0; index < 4; ++index)
        {
            bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8));
        }
    }

    void append_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value)
    {
        bytes.push_back(static_cast<std::uint8_t>(value));
        bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    }

    void append_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
    {
        for (int index = 0; index < 4; ++index)
        {
            bytes.push_back(static_cast<std::uint8_t>(value >> (index * 8)));
        }
    }

    void append_string(std::vector<std::uint8_t>& bytes, const char* value)
    {
        while (*value != '\0')
        {
            bytes.push_back(static_cast<std::uint8_t>(*value++));
        }
        bytes.push_back(0);
    }

    void write_empty_professions(std::vector<std::uint8_t>& bytes, std::size_t offset, bool active)
    {
        if (!active)
        {
            return;
        }
        const auto profession = offset + 201 * 2;
        write_u16(bytes, profession + 6, 1);
        write_u16(bytes, profession + 8 + 2, 1);
    }

    std::vector<std::uint8_t> make_craft_fixture()
    {
        std::vector<std::uint8_t> names;
        append_string(names, "");
        append_string(names, "Smithing");
        append_string(names, "Weapons");
        append_string(names, "Bronze Sword");
        append_string(names, "Metal");

        constexpr std::size_t header_size = 76;
        constexpr std::size_t profession_size = 8154;
        constexpr std::size_t recipe_size = 84;
        constexpr std::size_t category_size = 104;
        const auto profession0 = header_size + names.size();
        const auto recipe0 = profession0 + profession_size;
        const auto category0 = recipe0 + recipe_size * 2;
        const auto profession1 = category0 + category_size * 2;
        const auto profession2 = profession1 + profession_size;
        const auto file_size = profession2 + profession_size;

        std::vector<std::uint8_t> bytes(file_size, 0);
        write_u32(bytes, 4, static_cast<std::uint32_t>(names.size()));
        write_u32(bytes, 8, 5);
        write_u32(bytes, 12, 0);
        const std::array<std::size_t, 3> profession_offsets{profession0, profession1, profession2};
        for (std::size_t realm = 0; realm < 3; ++realm)
        {
            const auto header_offset = 16 + realm * 20;
            write_u32(bytes, header_offset, realm == 0 ? 2 : 0);
            write_u32(bytes, header_offset + 4, realm == 0 ? 2 : 0);
            write_u32(bytes, header_offset + 8, static_cast<std::uint32_t>(recipe0));
            write_u32(bytes, header_offset + 12, static_cast<std::uint32_t>(category0));
            write_u32(bytes, header_offset + 16, static_cast<std::uint32_t>(profession_offsets[realm]));
        }

        std::copy(names.begin(), names.end(), bytes.begin() + header_size);
        write_empty_professions(bytes, profession0, true);

        const auto recipe = recipe0 + recipe_size;
        write_u32(bytes, recipe, 3);
        write_u32(bytes, recipe + 4, 31);
        write_u32(bytes, recipe + 8, 99);
        write_u16(bytes, recipe + 14, 100);
        write_u16(bytes, recipe + 16, 1);
        write_u16(bytes, recipe + 18, 5);
        write_u32(bytes, recipe + 20, 4);
        write_u16(bytes, recipe + 24, 2);
        write_u16(bytes, recipe + 26, 32);

        const auto category = category0 + category_size;
        write_u32(bytes, category, 2);
        write_u16(bytes, category + 4, 1);
        return bytes;
    }

    void test_craft_recipe_binary()
    {
        const auto recipes = daoc_client_lib::parse_craft_recipes(make_craft_fixture());
        check(recipes.size() == 1, "craft recipe count mismatch");
        const auto& recipe = recipes.front();
        check(recipe.id == 99 && recipe.name == "Bronze Sword", "craft recipe identity mismatch");
        check(recipe.realm == "Albion" && recipe.profession == "Smithing", "craft recipe realm/profession mismatch");
        check(recipe.skill_level == 100 && recipe.item_level == 5 && recipe.material_level == 1,
              "craft recipe level fields mismatch");
        check(recipe.base_material_name == "Rowan" && recipe.ingredients.size() == 1,
              "craft recipe material decode mismatch");
        check(recipe.ingredients[0].full_name() == "Elm Metal" && recipe.ingredients[0].count == 2,
              "craft ingredient decode mismatch");
    }
}

int run_craft_recipes_tests()
{
    test_craft_recipe_binary();
    return 0;
}