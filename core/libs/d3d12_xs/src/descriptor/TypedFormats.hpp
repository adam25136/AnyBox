#pragma once

#include <d3d12.h>
#include <array>
#include <cstdint>

namespace AnyBox::Graphics {
struct TypedBufferFormat {
    DXGI_FORMAT format;
    std::uint32_t size;
    std::uint32_t encoding;
    std::array<std::uint32_t, 4> components;
};
inline constexpr std::array TypedBufferFormats{
    TypedBufferFormat{static_cast<DXGI_FORMAT>(2), 16, 0x4D, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(3), 16, 0x4B, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(4), 16, 0x4C, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(6), 12, 0x4A, {4, 5, 6, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(7), 12, 0x48, {4, 5, 6, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(8), 12, 0x49, {4, 5, 6, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(10), 8, 0x47, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(11), 8, 0x41, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(12), 8, 0x45, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(13), 8, 0x42, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(14), 8, 0x46, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(16), 8, 0x40, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(17), 8, 0x3E, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(18), 8, 0x3F, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(24), 4, 0x32, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(25), 4, 0x36, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(26), 4, 0x24, {4, 5, 6, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(28), 4, 0x38, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(30), 4, 0x3C, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(31), 4, 0x39, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(32), 4, 0x3D, {4, 5, 6, 7}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(34), 4, 0x1D, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(35), 4, 0x17, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(36), 4, 0x1B, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(37), 4, 0x18, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(38), 4, 0x1C, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(41), 4, 0x16, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(42), 4, 0x14, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(43), 4, 0x15, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(49), 2, 0xE, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(50), 2, 0x12, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(51), 2, 0xF, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(52), 2, 0x13, {4, 5, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(54), 2, 0xD, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(56), 2, 0x7, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(57), 2, 0xB, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(58), 2, 0x8, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(59), 2, 0xC, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(61), 1, 0x1, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(62), 1, 0x5, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(63), 1, 0x2, {4, 0, 0, 1}},
    TypedBufferFormat{static_cast<DXGI_FORMAT>(64), 1, 0x6, {4, 0, 0, 1}}
};
}
