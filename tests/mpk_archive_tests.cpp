#include "daoc_client_lib/mpk_archive.h"

#include <array>
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

    void test_legacy_archive_fixture()
    {
        constexpr std::array<unsigned char, 104> bytes{
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

        const auto path = std::filesystem::temp_directory_path() / "daoc_client_lib_fixture.mpk";
        {
            std::ofstream output(path, std::ios::binary);
            output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        }

        const daoc_client_lib::MpkArchive archive(path);
        std::filesystem::remove(path);

        check(archive.size() == 1, "MPK entry count mismatch");
        const auto* entry = archive.find("EXISTING.TXT");
        check(entry != nullptr, "case-insensitive MPK lookup failed");
        check(entry->name == "existing.txt", "MPK entry name mismatch");
        check(std::string(entry->data.begin(), entry->data.end()) == "Hello World !", "MPK payload mismatch");
        check(archive.find("missing.txt") == nullptr, "missing MPK lookup should return null");
    }
}

int run_mpk_archive_tests()
{
    test_legacy_archive_fixture();
    return 0;
}