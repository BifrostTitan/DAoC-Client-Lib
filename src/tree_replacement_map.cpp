#include "daoc_client_lib/tree_replacement_map.h"

#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace daoc_client_lib
{
    namespace
    {
        std::string lower_ascii(std::string value)
        {
            for (auto &character : value)
            {
                if (character >= 'A' && character <= 'Z')
                {
                    character = static_cast<char>(character + ('a' - 'A'));
                }
            }
            return value;
        }

        float parse_float(const std::string &value)
        {
            float parsed = 0.0f;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || !std::isfinite(parsed))
            {
                throw std::runtime_error("Invalid tree cluster coordinate: " + value);
            }
            return parsed;
        }

        std::int16_t parse_short(const std::string &value)
        {
            int parsed = 0;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || parsed < std::numeric_limits<std::int16_t>::min() || parsed > std::numeric_limits<std::int16_t>::max())
            {
                throw std::runtime_error("Invalid tree map height offset: " + value);
            }
            return static_cast<std::int16_t>(parsed);
        }

        std::string bytes_to_string(const std::vector<std::uint8_t> &bytes)
        {
            return {bytes.begin(), bytes.end()};
        }
    }

    TreeReplacementMap::TreeReplacementMap(const CsvTable &tree_map, const CsvTable &tree_clusters)
    {
        for (const auto &row : tree_map)
        {
            if (row.size() <= 4 || lower_ascii(row[0]) == "nif name")
            {
                continue;
            }
            try
            {
                TreeData data;
                data.real_nif = row[0];
                data.replacement = row[1];
                data.bark_texture = row[2];
                data.leaf_texture = row[3];
                data.offset_z = static_cast<float>(parse_short(row[4]));
                if (!tree_map_.emplace(lower_ascii(row[0]), std::move(data)).second)
                {
                    warnings_.push_back("Duplicate tree map key: " + lower_ascii(row[0]));
                }
            }
            catch (const std::exception &error)
            {
                warnings_.push_back("Could not parse tree map row '" + row[0] + "': " + error.what());
            }
        }

        for (const auto &row : tree_clusters)
        {
            if (row.size() <= 1)
            {
                continue;
            }
            const auto origin_name = lower_ascii(row[0]);
            if (origin_name == "name")
            {
                continue;
            }
            const auto target_name = lower_ascii(row[1]);
            const auto tree = tree_map_.find(target_name);

            float z_offset = 0.0f;
            std::string replacement;
            std::string bark_texture;
            std::string leaf_texture;
            if (tree != tree_map_.end())
            {
                z_offset = tree->second.offset_z;
                replacement = tree->second.replacement;
                bark_texture = tree->second.bark_texture;
                leaf_texture = tree->second.leaf_texture;
            }

            std::vector<TreeData> cluster;
            for (std::size_t index = 2; index + 2 < row.size(); index += 3)
            {
                try
                {
                    const auto x = parse_float(row[index]);
                    const auto y = parse_float(row[index + 1]);
                    const auto z = parse_float(row[index + 2]);
                    if (x == 0.0f && y == 0.0f && z == 0.0f)
                    {
                        continue;
                    }
                    cluster.push_back({target_name, x, y, z + z_offset, replacement, bark_texture, leaf_texture});
                }
                catch (const std::exception &error)
                {
                    warnings_.push_back("Could not parse tree cluster '" + origin_name + "' at index " + std::to_string(index) + ": " + error.what());
                }
            }
            if (cluster.empty())
            {
                warnings_.push_back("Empty tree cluster: " + origin_name);
                continue;
            }
            if (!tree_clusters_.emplace(origin_name, std::move(cluster)).second)
            {
                warnings_.push_back("Duplicate tree cluster key: " + origin_name);
            }
        }
    }

    TreeReplacementMap TreeReplacementMap::load(const ClientDataDirectory &client)
    {
        const auto map_bytes = client.read_package_file("treemap.mpk", "treemap.csv");
        const auto cluster_bytes = client.read_package_file("tree_clusters.mpk", "tree_clusters.csv");
        return TreeReplacementMap(
            parse_csv(bytes_to_string(map_bytes)), parse_csv(bytes_to_string(cluster_bytes)));
    }

    std::vector<TreeData> TreeReplacementMap::find(const std::string &nif_name) const
    {
        const auto key = lower_ascii(nif_name);
        const auto cluster = tree_clusters_.find(key);
        if (cluster != tree_clusters_.end())
        {
            return cluster->second;
        }
        const auto tree = tree_map_.find(key);
        return tree == tree_map_.end() ? std::vector<TreeData>{} : std::vector<TreeData>{tree->second};
    }

    std::vector<std::string> TreeReplacementMap::keys() const
    {
        std::map<std::string, bool> all_keys;
        for (const auto &[key, unused] : tree_clusters_)
        {
            static_cast<void>(unused);
            all_keys.emplace(key, true);
        }
        for (const auto &[key, unused] : tree_map_)
        {
            static_cast<void>(unused);
            all_keys.emplace(key, true);
        }
        std::vector<std::string> result;
        result.reserve(all_keys.size());
        for (const auto &[key, unused] : all_keys)
        {
            static_cast<void>(unused);
            result.push_back(key);
        }
        return result;
    }

    const std::vector<std::string> &TreeReplacementMap::warnings() const noexcept
    {
        return warnings_;
    }
}