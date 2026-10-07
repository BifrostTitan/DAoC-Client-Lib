int run_mpk_archive_tests();
int run_client_data_directory_tests();
int run_pcx_image_tests();
int run_text_data_tests();
int run_zone_data_tests();
int run_terrain_height_map_tests();
int run_zone_geometry_tests();
int run_river_geometry_tests();
int run_zone_records_tests();
int run_nif_geometry_tests();
int run_dungeon_geometry_tests();
int run_dungeon_water_tests();
int run_tree_replacement_map_tests();
int run_jump_point_tests();
int run_craft_recipes_tests();
int run_zone_geometry_chooser_tests();

int main()
{
    run_mpk_archive_tests();
    run_client_data_directory_tests();
    run_pcx_image_tests();
    run_text_data_tests();
    run_zone_data_tests();
    run_terrain_height_map_tests();
    run_zone_geometry_tests();
    run_river_geometry_tests();
    run_zone_records_tests();
    run_nif_geometry_tests();
    run_dungeon_geometry_tests();
    run_dungeon_water_tests();
    run_tree_replacement_map_tests();
    run_jump_point_tests();
    run_craft_recipes_tests();
    run_zone_geometry_chooser_tests();
}