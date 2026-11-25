#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>

namespace AnyBox::Descriptor {
inline std::uint32_t SamplerPositiveFixed(float value, double scale, std::uint32_t maximum) {
    if (std::bit_cast<std::uint32_t>(value) > 0x7F800000) return 0;
    return static_cast<std::uint32_t>(std::min(static_cast<double>(value) * scale, static_cast<double>(maximum)));
}
inline std::uint32_t SamplerSignedFixed(float value, double scale, std::uint32_t maximum) {
    auto bits = std::bit_cast<std::uint32_t>(value);
    auto magnitude = std::bit_cast<float>(bits & 0x7FFFFFFF);
    auto fixed = static_cast<std::int32_t>(SamplerPositiveFixed(magnitude, scale, maximum));
    return static_cast<std::uint32_t>((bits >> 31) != 0 ? -fixed : fixed);
}
}
