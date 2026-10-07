#include "daoc_client_lib/mpk_archive.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>
#include <zlib.h>

namespace daoc_client_lib
{
    namespace
    {
        constexpr std::uint32_t mpk_magic = 0x4b41504d;
        constexpr std::size_t archive_header_size = 16;
        constexpr std::size_t entry_header_size = 284;

        std::uint32_t read_u32(const std::vector<std::uint8_t> &bytes, std::size_t offset)
        {
            if (offset > bytes.size() || bytes.size() - offset < 4)
            {
                throw std::runtime_error("Truncated MPK integer field");
            }

            return static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
        }

        std::uint32_t crc32_of(const std::vector<std::uint8_t> &bytes)
        {
            uLong crc = crc32(0L, Z_NULL, 0);
            std::size_t offset = 0;
            while (offset < bytes.size())
            {
                const auto count = static_cast<uInt>(std::min<std::size_t>(
                    bytes.size() - offset, std::numeric_limits<uInt>::max()));
                crc = crc32(crc, bytes.data() + offset, count);
                offset += count;
            }
            return static_cast<std::uint32_t>(crc);
        }

        std::vector<std::uint8_t> inflate_data(
            const std::vector<std::uint8_t> &input,
            std::size_t expected_size,
            bool check_expected_size)
        {
            z_stream stream{};
            if (inflateInit(&stream) != Z_OK)
            {
                throw std::runtime_error("Could not initialize MPK decompression");
            }

            std::vector<std::uint8_t> output;
            if (check_expected_size)
            {
                output.reserve(expected_size);
            }

            std::array<std::uint8_t, 16384> buffer{};
            std::size_t input_offset = 0;
            int status = Z_OK;
            while (status == Z_OK)
            {
                if (stream.avail_in == 0 && input_offset < input.size())
                {
                    const auto count = static_cast<uInt>(std::min<std::size_t>(
                        input.size() - input_offset, std::numeric_limits<uInt>::max()));
                    stream.next_in = const_cast<Bytef *>(input.data() + input_offset);
                    stream.avail_in = count;
                    input_offset += count;
                }

                stream.next_out = buffer.data();
                stream.avail_out = static_cast<uInt>(buffer.size());
                status = inflate(&stream, Z_NO_FLUSH);
                const auto produced = buffer.size() - stream.avail_out;
                if (check_expected_size && produced > expected_size - output.size())
                {
                    inflateEnd(&stream);
                    throw std::runtime_error("MPK decompressed data exceeds its declared size");
                }
                output.insert(output.end(), buffer.begin(), buffer.begin() + produced);

                if (status == Z_OK && stream.avail_in == 0 && input_offset == input.size() && produced == 0)
                {
                    inflateEnd(&stream);
                    throw std::runtime_error("Truncated MPK compressed data");
                }
            }

            const auto consumed_all_input = stream.avail_in == 0 && input_offset == input.size();
            inflateEnd(&stream);
            if (status != Z_STREAM_END || !consumed_all_input)
            {
                throw std::runtime_error("Invalid MPK compressed data");
            }
            if (check_expected_size && output.size() != expected_size)
            {
                throw std::runtime_error("MPK decompressed data does not match its declared size");
            }
            return output;
        }

        std::string entry_name(const std::vector<std::uint8_t> &bytes, std::size_t offset)
        {
            const auto end = std::find(bytes.begin() + offset, bytes.begin() + offset + 256, 0);
            return std::string(bytes.begin() + offset, end);
        }

        std::string normalize_name(std::string name)
        {
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char character)
                           { return static_cast<char>(std::tolower(character)); });
            return name;
        }
    }

    MpkArchive::MpkArchive(const std::filesystem::path &path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            throw std::runtime_error("Could not open MPK file: " + path.string());
        }

        const std::istreambuf_iterator<char> input_begin(input);
        const std::istreambuf_iterator<char> input_end;
        std::vector<std::uint8_t> bytes(input_begin, input_end);
        if (bytes.size() < 5)
        {
            throw std::runtime_error("Truncated MPK header");
        }
        if (read_u32(bytes, 0) != mpk_magic)
        {
            throw std::runtime_error("Invalid MPK magic");
        }

        const std::size_t header_offset = 5;
        if (bytes.size() - header_offset < archive_header_size)
        {
            throw std::runtime_error("Truncated MPK archive header");
        }
        std::vector<std::uint8_t> header(archive_header_size);
        for (std::size_t index = 0; index < header.size(); ++index)
        {
            header[index] = bytes[header_offset + index] ^ static_cast<std::uint8_t>(index);
        }

        const auto expected_crc = read_u32(header, 0);
        const auto compressed_directory_size = read_u32(header, 4);
        const auto compressed_name_size = read_u32(header, 8);
        const auto entry_count = read_u32(header, 12);
        std::size_t offset = header_offset + archive_header_size;

        const auto has_bytes = [&bytes](std::size_t position, std::size_t count)
        {
            return position <= bytes.size() && count <= bytes.size() - position;
        };
        if (!has_bytes(offset, compressed_name_size))
        {
            throw std::runtime_error("Truncated MPK archive name");
        }
        std::vector<std::uint8_t> compressed_name(
            bytes.begin() + offset, bytes.begin() + offset + compressed_name_size);
        const auto decoded_name = inflate_data(compressed_name, 0, false);
        name_.assign(decoded_name.begin(), decoded_name.end());
        offset += compressed_name_size;

        if (!has_bytes(offset, compressed_directory_size))
        {
            throw std::runtime_error("Truncated MPK directory");
        }
        std::vector<std::uint8_t> compressed_directory(
            bytes.begin() + offset, bytes.begin() + offset + compressed_directory_size);
        if (crc32_of(compressed_directory) != expected_crc)
        {
            throw std::runtime_error("MPK directory CRC mismatch");
        }

        if (entry_count > std::numeric_limits<std::size_t>::max() / entry_header_size)
        {
            throw std::runtime_error("MPK directory entry count is too large");
        }
        const auto decoded_directory_size = static_cast<std::size_t>(entry_count) * entry_header_size;
        const auto directory = inflate_data(compressed_directory, decoded_directory_size, true);
        offset += compressed_directory_size;
        const auto data_start = offset;

        for (std::size_t index = 0; index < entry_count; ++index)
        {
            const auto record_offset = index * entry_header_size;
            const auto file_name = entry_name(directory, record_offset);
            const auto uncompressed_size = read_u32(directory, record_offset + 268);
            const auto data_offset = read_u32(directory, record_offset + 272);
            const auto compressed_size = read_u32(directory, record_offset + 276);
            const auto expected_file_crc = read_u32(directory, record_offset + 280);

            if (data_offset > bytes.size() - data_start || compressed_size > bytes.size() - data_start - data_offset)
            {
                throw std::runtime_error("MPK entry data is outside the archive");
            }
            const auto compressed_begin = bytes.begin() + data_start + data_offset;
            std::vector<std::uint8_t> compressed_data(
                compressed_begin, compressed_begin + compressed_size);
            if (crc32_of(compressed_data) != expected_file_crc)
            {
                throw std::runtime_error("MPK entry CRC mismatch: " + file_name);
            }

            auto data = inflate_data(compressed_data, uncompressed_size, true);
            auto key = normalize_name(file_name);
            if (!entries_.emplace(key, MpkEntry{file_name, std::move(data)}).second)
            {
                throw std::runtime_error("Duplicate MPK entry: " + file_name);
            }
        }
    }

    const std::string &MpkArchive::name() const noexcept
    {
        return name_;
    }

    std::size_t MpkArchive::size() const noexcept
    {
        return entries_.size();
    }

    const MpkEntry *MpkArchive::find(const std::string &name) const
    {
        const auto found = entries_.find(normalize_name(name));
        return found == entries_.end() ? nullptr : &found->second;
    }
}