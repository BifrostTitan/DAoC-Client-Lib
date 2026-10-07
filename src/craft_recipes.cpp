#include "daoc_client_lib/craft_recipes.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace daoc_client_lib
{
    namespace
    {
        constexpr std::size_t header_size = 16 + 3 * 20;
        constexpr std::size_t profession_count = 19;
        constexpr std::size_t profession_size = 201 * 2 + profession_count * (3 + 1 + 200) * 2;
        constexpr std::size_t category_size = 4 + 50 * 2;
        constexpr std::size_t material_count = 8;
        constexpr std::size_t recipe_size = 20 + material_count * 8;
        constexpr std::array<const char *, 96> base_materials{
            "", "", "", "", "", "", "", "Bronze", "Iron", "Mithril", "Asterite", "", "", "", "Alloy", "Steel",
            "", "Rawhide", "Tanned", "Cured", "Hard", "Rigid", "Embossed", "Imbued", "Runed", "Eldritch", "Fine Alloy", "", "", "Adamantium", "", "Rowan",
            "Elm", "Oak", "Ironwood", "Heartwood", "Runewood", "Stonewood", "Ebonwood", "Dyrwood", "", "Woolen", "Linen", "Brocade", "Silk", "Gossamer", "Sylvan", "Seamist",
            "Nightshade", "Wyvernskin", "Leaf", "Bone", "Vine", "Shell", "Fossil", "Amber", "Coral", "Chitin", "Copper", "Ferrite", "Quartz", "Dolomite", "Cobalt", "Carbide",
            "Sapphire", "Diamond", "Netherite", "Arcanite", "Netherium", "Arcanium", "Tempered", "Duskwood", "Silksteel", "Petrified", "Crystalized", "", "", "", "", "",
            "raw", "uncut", "rough", "flawed", "imperfect", "polished", "faceted", "precious", "flawless", "perfect", "", "", "", "", "", ""};

        struct RealmHeader
        {
            std::uint32_t recipe_count;
            std::uint32_t category_count;
            std::uint32_t recipe_offset;
            std::uint32_t category_offset;
            std::uint32_t profession_offset;
        };

        struct RawMaterial
        {
            std::uint32_t name_index;
            std::uint16_t count;
            std::uint16_t base_material;
        };

        struct RawRecipe
        {
            std::uint32_t name_index;
            std::uint32_t base_material;
            std::uint32_t id;
            std::uint16_t skill;
            std::uint16_t material_level;
            std::uint16_t level;
            std::array<RawMaterial, material_count> materials;
        };

        class BinaryReader
        {
        public:
            explicit BinaryReader(const std::vector<std::uint8_t> &data) : data_(data) {}

            void require(std::size_t offset, std::size_t count) const
            {
                if (offset > data_.size() || count > data_.size() - offset)
                {
                    throw std::runtime_error("Craft data is truncated or contains an invalid offset");
                }
            }

            std::uint16_t u16(std::size_t offset) const
            {
                require(offset, 2);
                return static_cast<std::uint16_t>(data_[offset]) | static_cast<std::uint16_t>(static_cast<std::uint16_t>(data_[offset + 1]) << 8);
            }

            std::uint32_t u32(std::size_t offset) const
            {
                require(offset, 4);
                return static_cast<std::uint32_t>(data_[offset]) | (static_cast<std::uint32_t>(data_[offset + 1]) << 8) | (static_cast<std::uint32_t>(data_[offset + 2]) << 16) | (static_cast<std::uint32_t>(data_[offset + 3]) << 24);
            }

            std::string c_string(std::size_t &offset, std::size_t end) const
            {
                if (offset >= end)
                {
                    throw std::runtime_error("Craft string list ended before its declared name count");
                }
                const auto begin = offset;
                while (offset < end && data_[offset] != 0)
                {
                    ++offset;
                }
                if (offset == end)
                {
                    throw std::runtime_error("Craft string is missing its null terminator");
                }
                std::string result(data_.begin() + begin, data_.begin() + offset);
                ++offset;
                return result;
            }

        private:
            const std::vector<std::uint8_t> &data_;
        };

        std::size_t checked_table_size(std::uint32_t count, std::size_t record_size)
        {
            if (count > std::numeric_limits<std::size_t>::max() / record_size)
            {
                throw std::runtime_error("Craft record count is too large");
            }
            return static_cast<std::size_t>(count) * record_size;
        }

        std::vector<std::string> read_names(
            const BinaryReader &reader,
            std::size_t offset,
            std::uint32_t byte_size,
            std::uint32_t name_count)
        {
            reader.require(offset, byte_size);
            if (name_count > byte_size)
            {
                throw std::runtime_error("Craft name count exceeds the declared string table size");
            }
            std::vector<std::string> names;
            names.reserve(name_count);
            const auto end = offset + byte_size;
            for (std::uint32_t index = 0; index < name_count; ++index)
            {
                names.push_back(reader.c_string(offset, end));
            }
            return names;
        }

        std::vector<std::uint16_t> read_profession_indexes(const BinaryReader &reader, std::uint32_t offset)
        {
            reader.require(offset, profession_size);
            std::vector<std::uint16_t> indexes;
            indexes.reserve(profession_count * 200);
            std::size_t cursor = static_cast<std::size_t>(offset) + 201 * 2;
            for (std::size_t profession = 0; profession < profession_count; ++profession)
            {
                const auto name_index = reader.u16(cursor + 6);
                indexes.push_back(name_index);
                for (std::size_t category = 0; category < 200; ++category)
                {
                    indexes.push_back(reader.u16(cursor + 8 + category * 2));
                }
                cursor += (3 + 1 + 200) * 2;
            }
            return indexes;
        }

        std::vector<RawRecipe> read_recipes(
            const BinaryReader &reader,
            const RealmHeader &realm)
        {
            reader.require(realm.recipe_offset, checked_table_size(realm.recipe_count, recipe_size));
            std::vector<RawRecipe> recipes;
            recipes.reserve(realm.recipe_count);
            for (std::uint32_t index = 0; index < realm.recipe_count; ++index)
            {
                const auto offset = static_cast<std::size_t>(realm.recipe_offset) + index * recipe_size;
                RawRecipe recipe{};
                recipe.name_index = reader.u32(offset);
                recipe.base_material = reader.u32(offset + 4);
                recipe.id = reader.u32(offset + 8);
                recipe.skill = reader.u16(offset + 14);
                recipe.material_level = reader.u16(offset + 16);
                recipe.level = reader.u16(offset + 18);
                auto material_offset = offset + 20;
                for (auto &material : recipe.materials)
                {
                    material.name_index = reader.u32(material_offset);
                    material.count = reader.u16(material_offset + 4);
                    material.base_material = reader.u16(material_offset + 6);
                    material_offset += 8;
                }
                recipes.push_back(recipe);
            }
            return recipes;
        }

        std::vector<std::pair<std::uint32_t, std::array<std::uint16_t, 50>>> read_categories(
            const BinaryReader &reader,
            const RealmHeader &realm)
        {
            reader.require(realm.category_offset, checked_table_size(realm.category_count, category_size));
            std::vector<std::pair<std::uint32_t, std::array<std::uint16_t, 50>>> categories;
            categories.reserve(realm.category_count);
            for (std::uint32_t index = 0; index < realm.category_count; ++index)
            {
                const auto offset = static_cast<std::size_t>(realm.category_offset) + index * category_size;
                std::pair<std::uint32_t, std::array<std::uint16_t, 50>> category{};
                category.first = reader.u32(offset);
                for (std::size_t recipe = 0; recipe < category.second.size(); ++recipe)
                {
                    category.second[recipe] = reader.u16(offset + 4 + recipe * 2);
                }
                categories.push_back(category);
            }
            return categories;
        }

        const std::string &get_name(const std::vector<std::string> &names, std::uint32_t index)
        {
            if (index >= names.size())
            {
                throw std::runtime_error("Craft record references a missing string");
            }
            return names[index];
        }

        std::optional<std::string> material_name(std::uint32_t index)
        {
            if (index == 0 || index >= base_materials.size() || base_materials[index][0] == '\0')
            {
                return std::nullopt;
            }
            return base_materials[index];
        }
    }

    std::string CraftIngredient::full_name() const
    {
        return base_material_name ? *base_material_name + " " + name : name;
    }

    std::string CraftRecipe::full_name() const
    {
        return base_material_name ? *base_material_name + " " + name : name;
    }

    std::vector<CraftRecipe> parse_craft_recipes(const std::vector<std::uint8_t> &bytes)
    {
        BinaryReader reader(bytes);
        reader.require(0, header_size);
        const auto name_list_size = reader.u32(4);
        const auto name_count = reader.u32(8);
        const auto name_list_offset = reader.u32(12);
        std::array<RealmHeader, 3> realms{};
        for (std::size_t index = 0; index < realms.size(); ++index)
        {
            const auto offset = 16 + index * 20;
            realms[index] = {
                reader.u32(offset), reader.u32(offset + 4), reader.u32(offset + 8),
                reader.u32(offset + 12), reader.u32(offset + 16)};
        }

        if (name_list_offset > std::numeric_limits<std::size_t>::max() - header_size)
        {
            throw std::runtime_error("Craft name list offset is too large");
        }
        const auto names = read_names(reader, header_size + name_list_offset, name_list_size, name_count);
        constexpr std::array<const char *, 3> realm_names{"Albion", "Midgard", "Hibernia"};
        std::array<std::vector<CraftRecipe>, 3> recipes_by_realm;

        for (std::size_t realm_index = 0; realm_index < realms.size(); ++realm_index)
        {
            const auto &realm = realms[realm_index];
            const auto profession_indexes = read_profession_indexes(reader, realm.profession_offset);
            const auto recipes = read_recipes(reader, realm);
            const auto categories = read_categories(reader, realm);

            for (std::size_t profession = 0; profession < profession_count; ++profession)
            {
                const auto profession_base = profession * 201;
                const auto profession_name_index = profession_indexes[profession_base];
                if (profession_name_index == 0)
                {
                    continue;
                }
                const auto &profession_name = get_name(names, profession_name_index);
                for (std::size_t category_index = 0; category_index < 200; ++category_index)
                {
                    if (profession_indexes[profession_base + 1 + category_index] == 0)
                    {
                        continue;
                    }
                    if (category_index >= categories.size())
                    {
                        throw std::runtime_error("Craft profession references a missing category");
                    }
                    const auto &category = categories[category_index];
                    if (category.first == 0)
                    {
                        continue;
                    }
                    static_cast<void>(get_name(names, category.first));
                    for (const auto recipe_index : category.second)
                    {
                        if (recipe_index == 0)
                        {
                            continue;
                        }
                        if (recipe_index >= recipes.size())
                        {
                            throw std::runtime_error("Craft category references a missing recipe");
                        }
                        const auto &raw = recipes[recipe_index];
                        if (raw.name_index == 0)
                        {
                            continue;
                        }
                        const auto &recipe_name = get_name(names, raw.name_index);
                        if (std::none_of(raw.materials.begin(), raw.materials.end(), [](const auto &material)
                                         { return material.count > 0; }))
                        {
                            continue;
                        }

                        CraftRecipe recipe;
                        recipe.id = static_cast<int>(raw.id);
                        recipe.name = recipe_name;
                        recipe.base_material_name = material_name(raw.base_material);
                        recipe.realm = realm_names[realm_index];
                        recipe.profession = profession_name;
                        recipe.skill_level = raw.skill;
                        recipe.item_level = raw.level;
                        recipe.material_level = raw.material_level;
                        for (const auto &raw_material : raw.materials)
                        {
                            if (raw_material.count <= 0 || raw_material.name_index == 0)
                            {
                                continue;
                            }
                            const auto &ingredient_name = get_name(names, raw_material.name_index);
                            if (ingredient_name.empty())
                            {
                                continue;
                            }
                            recipe.ingredients.push_back({ingredient_name,
                                                          material_name(raw_material.base_material),
                                                          raw_material.count,
                                                          std::nullopt});
                        }
                        recipes_by_realm[realm_index].push_back(std::move(recipe));
                    }
                }
            }
        }

        std::vector<CraftRecipe> result;
        for (auto &realm_recipes : recipes_by_realm)
        {
            for (auto &recipe : realm_recipes)
            {
                if (recipe.profession == "Basic Crafting")
                {
                    const auto matching_recipe = std::find_if(realm_recipes.begin(), realm_recipes.end(),
                                                              [&recipe](const auto &candidate)
                                                              {
                                                                  return candidate.name == recipe.name && candidate.material_level == recipe.material_level && candidate.profession != "Basic Crafting";
                                                              });
                    if (matching_recipe != realm_recipes.end())
                    {
                        recipe.base_material_name = matching_recipe->base_material_name;
                    }
                }

                for (auto &ingredient : recipe.ingredients)
                {
                    const auto sub_recipe = std::find_if(realm_recipes.begin(), realm_recipes.end(),
                                                         [&ingredient](const auto &candidate)
                                                         {
                                                             return candidate.full_name() == ingredient.full_name();
                                                         });
                    if (sub_recipe != realm_recipes.end())
                    {
                        ingredient.sub_recipe_id = sub_recipe->id;
                    }
                }
            }
            result.insert(result.end(), realm_recipes.begin(), realm_recipes.end());
        }
        return result;
    }

    std::vector<CraftRecipe> read_craft_recipes(const ClientDataDirectory &client)
    {
        return parse_craft_recipes(client.read_package_file("ifd.mpk", "tdl.crf"));
    }
}