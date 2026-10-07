#include "daoc_client_lib/client_data_directory.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace
{
    void check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void write_bytes(const std::filesystem::path& path, const unsigned char* bytes, std::size_t size)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream output(path, std::ios::binary);
        output.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(size));
    }

    void test_recursive_package_access()
    {
        constexpr std::array<unsigned char, 104> archive_bytes{
            0x4D, 0x50, 0x41, 0x4B, 0x02, 0xD8, 0x29, 0xD9,
            0x09, 0x21, 0x05, 0x06, 0x07, 0x1C, 0x09, 0x0A,
            0x0B, 0x0D, 0x0D, 0x0E, 0x0F, 0x78, 0xDA, 0x4B,
            0xAD, 0xC8, 0x2C, 0x2E, 0xC9, 0xCC, 0x4B, 0xD7,
            0xCB, 0x2D, 0xC8, 0x06, 0x00, 0x20, 0x75, 0x04,
            0xE2, 0x78, 0xDA, 0x4B, 0xAD, 0xC8, 0x2C, 0x2E,
            0xC9, 0xCC, 0x4B, 0xD7, 0x2B, 0xA9, 0x28, 0x61,
            0x18, 0x81, 0x40, 0x4D, 0xD4, 0x15, 0xEE, 0x6F,
            0x5E, 0x28, 0x2D, 0x0A, 0xC4, 0xD1, 0x46, 0xBB,
            0xD3, 0x01, 0x88, 0x9A, 0x07, 0xBF, 0x78, 0xDA,
            0xF3, 0x48, 0xCD, 0xC9, 0xC9, 0x57, 0x08, 0xCF,
            0x2F, 0xCA, 0x49, 0x51, 0x50, 0x04, 0x00, 0x20,
            0xA6, 0x04, 0x5E
        };

        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto root = std::filesystem::temp_directory_path() / ("daoc-client-test-" + std::to_string(unique));
        std::filesystem::create_directories(root);
        std::ofstream(root / "camelot.exe").put('\0');
        std::ofstream(root / "game.dll").put('\0');
        write_bytes(root / "zone001" / "dat001.mpk", archive_bytes.data(), archive_bytes.size());

        const daoc_client_lib::ClientDataDirectory client(root);
        const auto entry = client.read_package_file("DAT001.MPK", "EXISTING.TXT");
        check(std::string(entry.begin(), entry.end()) == "Hello World !", "client package lookup failed");
        check(client.files().size() == 3, "client file filtering mismatch");
        std::filesystem::remove_all(root);
    }
}

int run_client_data_directory_tests()
{
    test_recursive_package_access();
    return 0;
}