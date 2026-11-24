#pragma once

#include <bit>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <array>

namespace BufferTests::Reference {
inline std::uint32_t MinMax(std::uint32_t left, std::uint32_t right, bool maximum) {
    auto flush = [](std::uint32_t value) { return (value & 0x7F800000U) == 0 ? value & 0x80000000U : value; };
    left = flush(left);
    right = flush(right);
    auto a = std::bit_cast<float>(left);
    auto b = std::bit_cast<float>(right);
    if (std::isnan(a) && std::isnan(b)) return 0x7FC00000U;
    if (std::isnan(a)) return right;
    if (std::isnan(b)) return left;
    if (((left | right) & 0x7FFFFFFFU) == 0) return maximum ? left & right : left | right;
    return std::bit_cast<std::uint32_t>(maximum ? std::fmax(a, b) : std::fmin(a, b));
}

inline std::uint32_t Median(std::uint32_t first, std::uint32_t second, std::uint32_t third) {
    std::array values{first, second, third};
    for (auto& value : values) {
        if (std::isnan(std::bit_cast<float>(value))) return MinMax(MinMax(first, second, false), third, false);
        if ((value & 0x7F800000U) == 0) value &= 0x80000000U;
    }
    std::sort(values.begin(), values.end(), [](auto left, auto right) {
        if (((left | right) & 0x7FFFFFFFU) == 0) return left > right;
        return std::bit_cast<float>(left) < std::bit_cast<float>(right);
    });
    return values[1];
}
}
