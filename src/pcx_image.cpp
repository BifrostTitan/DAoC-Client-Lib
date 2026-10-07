#include "daoc_client_lib/pcx_image.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace daoc_client_lib
{
    namespace
    {
        constexpr std::size_t pcx_header_size = 128;
        constexpr std::size_t palette_size = 256 * 3;

        std::uint16_t read_u16(const std::vector<std::uint8_t> &bytes, std::size_t offset)
        {
            return static_cast<std::uint16_t>(bytes[offset]) | static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
        }

        std::vector<std::uint8_t> decode_scan_data(
            const std::vector<std::uint8_t> &bytes,
            std::size_t position,
            std::size_t end,
            std::size_t expected_size,
            bool rle)
        {
            if (end < position)
            {
                throw std::runtime_error("Invalid PCX image data bounds");
            }

            if (!rle)
            {
                if (end - position < expected_size)
                {
                    throw std::runtime_error("Truncated PCX image data");
                }
                return std::vector<std::uint8_t>(
                    bytes.begin() + position, bytes.begin() + position + expected_size);
            }

            std::vector<std::uint8_t> decoded;
            decoded.reserve(expected_size);
            while (decoded.size() < expected_size)
            {
                if (position >= end)
                {
                    throw std::runtime_error("Truncated PCX RLE data");
                }

                const auto code = bytes[position++];
                std::size_t count = 1;
                std::uint8_t value = code;
                if ((code & 0xc0) == 0xc0)
                {
                    count = code & 0x3f;
                    if (count == 0 || position >= end)
                    {
                        throw std::runtime_error("Invalid PCX RLE run");
                    }
                    value = bytes[position++];
                }

                if (count > expected_size - decoded.size())
                {
                    throw std::runtime_error("PCX RLE run exceeds the image dimensions");
                }
                decoded.insert(decoded.end(), count, value);
            }
            return decoded;
        }
    }

    PcxImage PcxImage::decode(const std::vector<std::uint8_t> &bytes)
    {
        if (bytes.size() < pcx_header_size)
        {
            throw std::runtime_error("PCX file is shorter than its header");
        }
        if (bytes[0] != 0x0a)
        {
            throw std::runtime_error("Invalid PCX manufacturer byte");
        }

        const auto encoding = bytes[2];
        const auto bits_per_pixel = bytes[3];
        const auto x_min = read_u16(bytes, 4);
        const auto y_min = read_u16(bytes, 6);
        const auto x_max = read_u16(bytes, 8);
        const auto y_max = read_u16(bytes, 10);
        const auto planes = bytes[65];
        const auto bytes_per_line = read_u16(bytes, 66);

        if (encoding > 1 || bits_per_pixel != 8 || (planes != 1 && planes != 3))
        {
            throw std::runtime_error("Unsupported PCX encoding or color layout");
        }
        if (x_max < x_min || y_max < y_min)
        {
            throw std::runtime_error("Invalid PCX image bounds");
        }

        PcxImage image;
        image.width_ = static_cast<std::size_t>(x_max) - x_min + 1;
        image.height_ = static_cast<std::size_t>(y_max) - y_min + 1;
        if (bytes_per_line < image.width_)
        {
            throw std::runtime_error("PCX scanline is shorter than the image width");
        }
        if (image.width_ > std::numeric_limits<std::size_t>::max() / image.height_)
        {
            throw std::runtime_error("PCX image dimensions are too large");
        }

        const auto pixels_count = image.width_ * image.height_;
        if (pixels_count > image.pixels_.max_size())
        {
            throw std::runtime_error("PCX image dimensions are too large");
        }
        if (static_cast<std::size_t>(bytes_per_line) > std::numeric_limits<std::size_t>::max() / image.height_ / planes)
        {
            throw std::runtime_error("PCX scanline data is too large");
        }
        const auto decoded_size = static_cast<std::size_t>(bytes_per_line) * image.height_ * planes;

        std::size_t image_data_end = bytes.size();
        std::vector<RgbPixel> palette;
        if (planes == 1)
        {
            if (bytes.size() < pcx_header_size + palette_size + 1)
            {
                throw std::runtime_error("PCX indexed image is missing its palette");
            }
            const auto palette_marker = bytes.size() - palette_size - 1;
            if (bytes[palette_marker] != 0x0c)
            {
                throw std::runtime_error("PCX indexed image has no VGA palette marker");
            }
            image_data_end = palette_marker;
            palette.resize(256);
            for (std::size_t index = 0; index < palette.size(); ++index)
            {
                const auto palette_offset = palette_marker + 1 + index * 3;
                palette[index] = {bytes[palette_offset], bytes[palette_offset + 1], bytes[palette_offset + 2]};
            }
        }

        const auto decoded = decode_scan_data(
            bytes, pcx_header_size, image_data_end, decoded_size, encoding == 1);
        image.pixels_.resize(pixels_count);
        for (std::size_t y = 0; y < image.height_; ++y)
        {
            const auto row_offset = y * bytes_per_line * planes;
            for (std::size_t x = 0; x < image.width_; ++x)
            {
                if (planes == 1)
                {
                    image.pixels_[y * image.width_ + x] = palette[decoded[row_offset + x]];
                }
                else
                {
                    image.pixels_[y * image.width_ + x] = {
                        decoded[row_offset + x],
                        decoded[row_offset + bytes_per_line + x],
                        decoded[row_offset + bytes_per_line * 2 + x]};
                }
            }
        }
        return image;
    }

    std::size_t PcxImage::width() const noexcept
    {
        return width_;
    }

    std::size_t PcxImage::height() const noexcept
    {
        return height_;
    }

    const RgbPixel &PcxImage::pixel(std::size_t x, std::size_t y) const
    {
        if (x >= width_ || y >= height_)
        {
            throw std::out_of_range("PCX pixel coordinates are outside the image");
        }
        return pixels_[y * width_ + x];
    }
}