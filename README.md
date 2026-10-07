# DAoC Client Library (Native)

This C++17 library ports the non-rendering DAoC client data layer. It has links to the sibling `daoc-niflib` project for NIF loading.

The current port includes MPK reading, client directory discovery, PCX decoding, CSV/DAT parsing, zone catalogs and geometry records, terrain height sampling, NIF placement metadata, tree replacement data, jump points, and craft recipes.

## Build

Configure from the `DaocClientLib` directory so CMake also adds the sibling `daoc-niflib` project:

```powershell
cmake -S . -B build -DDAOC_CLIENT_LIB_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The standalone build requires ZLIB. Unreal Build Tool integration is not included yet; the source is engine-independent and will need to be added to an Unreal module with the engine's Zlib module and the matching `daoc-niflib` sources/library.