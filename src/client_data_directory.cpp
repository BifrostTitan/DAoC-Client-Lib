#include "daoc_client_lib/client_data_directory.h"

#include "daoc_client_lib/mpk_archive.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace daoc_client_lib
{
    namespace
    {
        std::string lower_ascii(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
                           { return static_cast<char>(std::tolower(character)); });
            return value;
        }

        bool is_client_file(const std::filesystem::path &path)
        {
            const auto filename = lower_ascii(path.filename().string());
            if (filename == "camelot.exe" || filename == "game.dll")
            {
                return true;
            }

            const auto extension = lower_ascii(path.extension().string());
            constexpr const char *supported_extensions[] = {
                ".mpk", ".npk", ".nif", ".nhd", ".pcx", ".tga", ".bmp", ".dds", ".csv"};
            return std::any_of(std::begin(supported_extensions), std::end(supported_extensions),
                               [&extension](const char *candidate)
                               { return extension == candidate; });
        }

        std::vector<std::uint8_t> read_file_bytes(const std::filesystem::path &path)
        {
            std::ifstream input(path, std::ios::binary);
            if (!input)
            {
                throw std::runtime_error("Could not read client file: " + path.string());
            }
            return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        }
    }

    ClientDataDirectory::ClientDataDirectory(const std::filesystem::path &root)
    {
        if (root.empty())
        {
            throw std::invalid_argument("Client directory path cannot be empty");
        }
        if (!std::filesystem::is_directory(root))
        {
            throw std::runtime_error("Client directory does not exist: " + root.string());
        }

        root_ = std::filesystem::weakly_canonical(root);
        bool has_camelot = false;
        bool has_game_dll = false;
        for (const auto &entry : std::filesystem::directory_iterator(root_))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }
            const auto filename = lower_ascii(entry.path().filename().string());
            has_camelot = has_camelot || filename == "camelot.exe";
            has_game_dll = has_game_dll || filename == "game.dll";
        }
        if (!has_camelot || !has_game_dll)
        {
            throw std::runtime_error("Directory is missing camelot.exe or game.dll: " + root_.string());
        }

        for (const auto &entry : std::filesystem::recursive_directory_iterator(root_))
        {
            if (entry.is_regular_file() && is_client_file(entry.path()))
            {
                files_.push_back(entry.path());
            }
        }
        std::sort(files_.begin(), files_.end());
    }

    const std::filesystem::path &ClientDataDirectory::root() const noexcept
    {
        return root_;
    }

    const std::vector<std::filesystem::path> &ClientDataDirectory::files() const noexcept
    {
        return files_;
    }

    std::vector<std::uint8_t> ClientDataDirectory::read_package_file(
        const std::string &package_name,
        const std::string &entry_name) const
    {
        const auto data = try_read_package_file(package_name, entry_name);
        if (!data)
        {
            throw std::runtime_error("Package or entry not found: " + package_name + "/" + entry_name);
        }
        return *data;
    }

    std::optional<std::vector<std::uint8_t>> ClientDataDirectory::try_read_package_file(
        const std::string &package_name,
        const std::string &entry_name) const
    {
        if (package_name.empty() || entry_name.empty())
        {
            throw std::invalid_argument("Package and entry names must not be empty");
        }

        const auto normalized_package = lower_ascii(package_name);
        const auto package = std::find_if(files_.begin(), files_.end(), [&normalized_package](const auto &path)
                                          { return lower_ascii(path.filename().string()) == normalized_package; });
        if (package == files_.end())
        {
            return std::nullopt;
        }

        const MpkArchive archive(*package);
        const auto *entry = archive.find(entry_name);
        if (entry == nullptr)
        {
            return std::nullopt;
        }
        return entry->data;
    }

    std::optional<std::vector<std::uint8_t>> ClientDataDirectory::search_raw_or_packed(
        const std::string &file_name,
        const std::vector<std::string> &package_extensions) const
    {
        if (file_name.empty())
        {
            throw std::invalid_argument("Client file name must not be empty");
        }

        const auto normalized_file = lower_ascii(file_name);
        const auto raw_file = std::find_if(files_.begin(), files_.end(), [&normalized_file](const auto &path)
                                           { return lower_ascii(path.filename().string()) == normalized_file; });
        if (raw_file != files_.end())
        {
            return read_file_bytes(*raw_file);
        }

        const std::filesystem::path requested_path(file_name);
        const auto package_stem = lower_ascii(requested_path.stem().string());
        for (const auto &path : files_)
        {
            if (lower_ascii(path.stem().string()) != package_stem)
            {
                continue;
            }
            const auto extension = lower_ascii(path.extension().string());
            const auto allowed = std::any_of(package_extensions.begin(), package_extensions.end(),
                                             [&extension](const auto &candidate)
                                             { return lower_ascii(candidate) == extension; });
            if (!allowed)
            {
                continue;
            }

            const MpkArchive archive(path);
            if (const auto *entry = archive.find(file_name))
            {
                return entry->data;
            }
        }
        return std::nullopt;
    }
}