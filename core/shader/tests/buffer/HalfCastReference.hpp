#pragma once

#include "HalfPackReference.hpp"

namespace BufferTests {
inline std::uint32_t RoundHalfReference(std::uint32_t bits) {
    auto sign = (bits >> 16) & 0x8000U;
    auto magnitude = bits & 0x7FFFFFFFU;
    if (magnitude > 0x7F800000) return 0x7E00;
    auto value = std::abs(static_cast<double>(std::bit_cast<float>(bits)));
    if (value >= 65520) return sign | 0x7C00;
    auto round = [](double input) {
        auto truncated = static_cast<std::uint32_t>(input);
        auto remainder = input - truncated;
        return truncated + static_cast<std::uint32_t>(remainder > 0.5 || (remainder == 0.5 && (truncated & 1) != 0));
    };
    if (value < 0x1p-14) return sign | round(std::ldexp(value, 24));
    int exponent;
    auto fraction = std::frexp(value, &exponent);
    return sign | ((static_cast<std::uint32_t>(exponent + 14) << 10) + round(std::ldexp(fraction, 11)) - 1024);
}

inline std::uint32_t CanonicalHalf(std::uint32_t bits) { return (bits & 0x7FFFU) > 0x7C00U ? 0x7E00U : bits; }
inline std::uint32_t WidenCanonicalHalf(std::uint32_t bits) { return (bits & 0x7FFFU) > 0x7C00U ? 0x7FC00000U : WidenHalfReference(bits); }
}
