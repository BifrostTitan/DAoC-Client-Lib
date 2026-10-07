#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace daoc_client_lib
{
    struct RgbPixel
    {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;
    };

    class PcxImage
    {
    public:
        static PcxImage decode(const std::vector<std::uint8_t>& bytes);

        std::size_t width() const noexcept;
        std::size_t height() const noexcept;
        const RgbPixel& pixel(std::size_t x, std::size_t y) const;

    private:
        std::size_t width_ = 0;
        std::size_t height_ = 0;
        std::vector<RgbPixel> pixels_;
    };
}