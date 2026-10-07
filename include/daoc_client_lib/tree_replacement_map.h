#pragma once

#include "daoc_client_lib/client_data_directory.h"
#include "daoc_client_lib/text_data.h"

#include <map>
#include <string>
#include <vector>

namespace daoc_client_lib
{
    struct TreeData
    {
        std::string real_nif;
        float offset_x = 0.0f;
        float offset_y = 0.0f;
        float offset_z = 0.0f;
        std::string replacement;
        std::string bark_texture;
        std::string leaf_texture;
    };

    class TreeReplacementMap
    {
    public:
        TreeReplacementMap(const CsvTable& tree_map, const CsvTable& tree_clusters);
        static TreeReplacementMap load(const ClientDataDirectory& client);

        std::vector<TreeData> find(const std::string& nif_name) const;
        std::vector<std::string> keys() const;
        const std::vector<std::string>& warnings() const noexcept;

    private:
        std::map<std::string, TreeData> tree_map_;
        std::map<std::string, std::vector<TreeData>> tree_clusters_;
        std::vector<std::string> warnings_;
    };
}