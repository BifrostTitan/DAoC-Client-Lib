#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace daoc_client_lib
{
    class ClientDataDirectory
    {
    public:
        explicit ClientDataDirectory(const std::filesystem::path& root);

        const std::filesystem::path& root() const noexcept;
        const std::vector<std::filesystem::path>& files() const noexcept;
        std::vector<std::uint8_t> read_package_file(
            const std::string& package_name,
            const std::string& entry_name) const;
        std::optional<std::vector<std::uint8_t>> try_read_package_file(
            const std::string& package_name,
            const std::string& entry_name) const;
        std::optional<std::vector<std::uint8_t>> search_raw_or_packed(
            const std::string& file_name,
            const std::vector<std::string>& package_extensions = {".mpk", ".npk"}) const;

    private:
        std::filesystem::path root_;
        std::vector<std::filesystem::path> files_;
    };
}