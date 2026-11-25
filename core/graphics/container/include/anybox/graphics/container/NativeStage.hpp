#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyBox::Graphics::Container {
enum class NativeHardwareStage : std::uint8_t { Vertex = 4, Pixel = 5, Compute = 6 };
struct NativeStageState {
    NativeHardwareStage stage;
    std::uint32_t floatControl;
    bool dx10Clamp;
    bool ieeeMode;
};
NativeStageState ReadNativeStageState(std::span<const std::byte> program);
}
