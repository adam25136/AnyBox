#pragma once

#include "HalfPackReference.hpp"

namespace BufferTests {
inline std::uint32_t HalfMinMaxReference(std::uint32_t left, std::uint32_t right, bool maximum) {
    auto a = std::bit_cast<float>(WidenHalfReference(left));
    auto b = std::bit_cast<float>(WidenHalfReference(right));
    if (std::isnan(b)) return left;
    if (std::isnan(a)) return right;
    if (a == 0 && b == 0) return maximum ? left & right : left | right;
    return (maximum ? a > b : a < b) ? left : right;
}
}
