#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace daoc_client_lib
{
    struct MpkEntry
    {
        std::string name;
        std::vector<std::uint8_t> data;
    };

    class MpkArchive
    {
    public:
        explicit MpkArchive(const std::filesystem::path& path);

        const std::string& name() const noexcept;
        std::size_t size() const noexcept;
        const MpkEntry* find(const std::string& name) const;

    private:
        std::string name_;
        std::unordered_map<std::string, MpkEntry> entries_;
    };
}