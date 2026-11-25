#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyBox::Graphics::Container {
struct NativeComputeState {
    std::array<std::uint32_t, 3> threads;
    std::uint32_t resourceControl;
    std::uint32_t declaredSharedBytes;
    std::uint32_t floatControl;
    bool dx10Clamp;
    bool ieeeMode;
};

NativeComputeState ReadNativeComputeState(std::span<const std::byte> program);
}
