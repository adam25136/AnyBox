#pragma once

#include <bit>
#include <cmath>
#include <cstdint>

namespace BufferTests {
inline std::uint32_t HalfPackReference(std::uint32_t bits) {
    auto sign = (bits >> 16) & 0x8000;
    auto fraction = bits & 0x7FFFFF;
    if ((bits & 0x7F800000) == 0x7F800000) return sign | (fraction ? 0x7E00 | (fraction >> 13) : 0x7C00);
    double magnitude = std::abs(static_cast<double>(std::bit_cast<float>(bits)));
    if (magnitude >= 65504) return sign | 0x7BFF;
    if (magnitude < std::ldexp(1.0, -14)) return sign | static_cast<std::uint32_t>(std::ldexp(magnitude, 24));
    int exponent;
    auto mantissa = std::frexp(magnitude, &exponent);
    return sign | (static_cast<std::uint32_t>(exponent + 14) << 10) | (static_cast<std::uint32_t>(std::ldexp(mantissa, 11)) - 1024);
}

inline std::uint32_t WidenHalfReference(std::uint32_t half) {
    auto exponent = (half >> 10) & 31;
    auto fraction = half & 1023;
    if (exponent == 31) return ((half & 0x8000) << 16) | 0x7F800000 | (fraction << 13);
    double magnitude = exponent ? std::ldexp(static_cast<double>(1024 + fraction), static_cast<int>(exponent) - 25) : std::ldexp(static_cast<double>(fraction), -24);
    auto value = static_cast<float>((half & 0x8000) ? -magnitude : magnitude);
    return std::bit_cast<std::uint32_t>(value);
}
}
