#pragma once

#include "daoc_client_lib/client_data_directory.h"

#include <optional>
#include <string>
#include <vector>

namespace daoc_client_lib
{
    struct CraftIngredient
    {
        std::string name;
        std::optional<std::string> base_material_name;
        int count = 0;
        std::optional<int> sub_recipe_id;

        std::string full_name() const;
    };

    struct CraftRecipe
    {
        int id = 0;
        std::string name;
        std::optional<std::string> base_material_name;
        std::string realm;
        std::string profession;
        int skill_level = 0;
        int item_level = 0;
        int material_level = 0;
        std::vector<CraftIngredient> ingredients;

        std::string full_name() const;
    };

    std::vector<CraftRecipe> parse_craft_recipes(const std::vector<std::uint8_t> &bytes);
    std::vector<CraftRecipe> read_craft_recipes(const ClientDataDirectory &client);
}