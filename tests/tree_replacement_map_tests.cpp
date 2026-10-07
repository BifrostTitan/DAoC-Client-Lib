#include "daoc_client_lib/tree_replacement_map.h"

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

    void test_tree_lookup_and_cluster_inheritance()
    {
        const auto map = daoc_client_lib::parse_csv(
            "NIF Name,Replacement,Bark,Leaf,Z\nOak.nif,oak.spt,bark.dds,leaf.dds,5\n");
        const auto clusters = daoc_client_lib::parse_csv(
            "OakCluster,Oak.nif,1.5,2.5,3,0,0,0\n");
        const daoc_client_lib::TreeReplacementMap replacements(map, clusters);
        const auto trees = replacements.find("OAKCLUSTER");
        check(trees.size() == 1, "tree cluster lookup mismatch");
        check(trees[0].real_nif == "oak.nif" && trees[0].offset_x == 1.5f && trees[0].offset_y == 2.5f,
              "tree cluster data mismatch");
        check(trees[0].offset_z == 8.0f && trees[0].replacement == "oak.spt",
              "tree cluster inherited overrides mismatch");
        check(replacements.find("oak.nif").size() == 1, "single-tree lookup mismatch");
        check(replacements.find("missing.nif").empty(), "missing tree lookup mismatch");
        check(replacements.keys().size() == 2, "tree replacement key union mismatch");
    }
}

int run_tree_replacement_map_tests()
{
    test_tree_lookup_and_cluster_inheritance();
    return 0;
}