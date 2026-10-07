find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    include(FetchContent)
    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG v1.3.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(zlib)
endif()

if(TARGET ZLIB::ZLIB)
    set(DAOC_ZLIB_TARGET ZLIB::ZLIB)
elseif(TARGET zlibstatic)
    set(DAOC_ZLIB_TARGET zlibstatic)
else()
    message(FATAL_ERROR "No usable zlib target was provided")
endif()

add_library(daoc_client_lib STATIC
    src/client_data_directory.cpp
    src/craft_recipes.cpp
    src/dungeon_water.cpp
    src/jump_point.cpp
    src/nif_geometry.cpp
    src/mpk_archive.cpp
    src/nif_asset.cpp
    src/pcx_image.cpp
    src/river_geometry.cpp
    src/terrain_height_map.cpp
    src/text_data.cpp
    src/tree_replacement_map.cpp
    src/zone_records.cpp
    src/zone_geometry.cpp
    src/zone_data.cpp
    src/zone_geometry_chooser.cpp
)

add_library(DaocClientLib::DaocClientLib ALIAS daoc_client_lib)

add_executable(daoc_zone_exporter tools/zone_exporter.cpp)
target_link_libraries(daoc_zone_exporter PRIVATE DaocClientLib::DaocClientLib)

target_compile_features(daoc_client_lib PUBLIC cxx_std_17)
target_include_directories(daoc_client_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(daoc_client_lib
    PUBLIC Niflib::niflib
    PRIVATE ${DAOC_ZLIB_TARGET}
)

if(MSVC)
    target_compile_options(daoc_client_lib PRIVATE /W4 /permissive-)
else()
    target_compile_options(daoc_client_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(DAOC_CLIENT_LIB_BUILD_TESTS "Build DaocClientLib native tests" ON)
if(DAOC_CLIENT_LIB_BUILD_TESTS)
    enable_testing()
    add_executable(daoc_client_lib_tests
        tests/native_tests.cpp
        tests/mpk_archive_tests.cpp
        tests/client_data_directory_tests.cpp
        tests/pcx_image_tests.cpp
        tests/text_data_tests.cpp
        tests/zone_data_tests.cpp
        tests/terrain_height_map_tests.cpp
        tests/zone_geometry_tests.cpp
        tests/river_geometry_tests.cpp
        tests/zone_records_tests.cpp
        tests/nif_geometry_tests.cpp
        tests/dungeon_geometry_tests.cpp
        tests/dungeon_water_tests.cpp
        tests/tree_replacement_map_tests.cpp
        tests/jump_point_tests.cpp
        tests/craft_recipes_tests.cpp
        tests/zone_geometry_chooser_tests.cpp
    )
    target_link_libraries(daoc_client_lib_tests PRIVATE DaocClientLib::DaocClientLib)
    add_test(NAME daoc_client_lib_tests COMMAND daoc_client_lib_tests)
endif()