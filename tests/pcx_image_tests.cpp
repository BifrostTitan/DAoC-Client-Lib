#include "daoc_client_lib/pcx_image.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace
{
    void check(bool condition, const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void write_u16(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t value)
    {
        bytes[offset] = static_cast<std::uint8_t>(value);
        bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8);
    }

    std::vector<std::uint8_t> pcx_header(std::uint8_t encoding, std::uint8_t planes)
    {
        std::vector<std::uint8_t> bytes(128, 0);
        bytes[0] = 0x0a;
        bytes[1] = 5;
        bytes[2] = encoding;
        bytes[3] = 8;
        write_u16(bytes, 8, 1);
        bytes[65] = planes;
        write_u16(bytes, 66, 2);
        return bytes;
    }

    void test_indexed_rle_palette()
    {
        auto bytes = pcx_header(1, 1);
        bytes.insert(bytes.end(), {0xc2, 1});
        bytes.push_back(0x0c);
        bytes.resize(bytes.size() + 256 * 3, 0);
        bytes[131] = 64;
        bytes[132] = 128;
        bytes[133] = 192;

        const auto image = daoc_client_lib::PcxImage::decode(bytes);
        check(image.width() == 2 && image.height() == 1, "indexed PCX dimensions mismatch");
        const auto& pixel = image.pixel(1, 0);
        check(pixel.red == 64 && pixel.green == 128 && pixel.blue == 192, "indexed PCX palette mismatch");
    }

    void test_planar_rgb()
    {
        auto bytes = pcx_header(0, 3);
        bytes.insert(bytes.end(), {10, 20, 30, 40, 50, 60});

        const auto image = daoc_client_lib::PcxImage::decode(bytes);
        const auto& first = image.pixel(0, 0);
        const auto& second = image.pixel(1, 0);
        check(first.red == 10 && first.green == 30 && first.blue == 50, "first planar PCX pixel mismatch");
        check(second.red == 20 && second.green == 40 && second.blue == 60, "second planar PCX pixel mismatch");
    }
}

int run_pcx_image_tests()
{
    test_indexed_rle_palette();
    test_planar_rgb();
    return 0;
}