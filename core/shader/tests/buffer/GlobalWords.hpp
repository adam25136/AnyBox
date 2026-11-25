#pragma once

#include <array>
#include <cstdint>

namespace BufferTests {
inline constexpr std::array<std::uint32_t, 8> ConstantWords{0, 0x80000000, 0x7FC12345, 0x7F812345, 0x7F800000, 0xFF800000, 0x12345678, UINT32_MAX};
}
