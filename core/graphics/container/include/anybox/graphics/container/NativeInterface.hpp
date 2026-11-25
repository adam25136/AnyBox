#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyBox::Graphics::Container {
struct NativePixelAttribute {
    std::uint8_t kind;
    std::uint8_t registerIndex;
    std::uint8_t componentMask;
    std::uint8_t interpolation;
};

struct NativePixelInputs {
    std::array<NativePixelAttribute, 32> attributes{};
    std::uint32_t count = 0;
    std::uint32_t inputEnable = 0;
};

struct NativeVertexParameter {
    std::uint8_t kind;
    std::uint8_t registerIndex;
};

struct NativeVertexOutputs {
    std::array<NativeVertexParameter, 32> parameters{};
    std::uint32_t count = 0;
};

NativePixelInputs ReadNativePixelInputs(std::span<const std::byte> program);
NativeVertexOutputs ReadNativeVertexOutputs(std::span<const std::byte> program);
}
