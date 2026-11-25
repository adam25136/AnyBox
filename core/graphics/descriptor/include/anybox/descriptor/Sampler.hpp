#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace AnyBox::Descriptor {
struct SamplerDescription {
    std::uint32_t Flags = 0;
    std::uint32_t MagFilter = 1;
    std::uint32_t MinFilter = 1;
    std::uint32_t MipFilter = 2;
    std::uint32_t ZFilter = 2;
    std::uint32_t ReductionMode = 0;
    std::uint32_t AddressU = 3;
    std::uint32_t AddressV = 3;
    std::uint32_t AddressW = 3;
    float MinLod = std::numeric_limits<float>::lowest();
    float MaxLod = std::numeric_limits<float>::max();
    float MipLodBias = 0;
    float MipLodBiasSecondary = 0;
    std::uint32_t MaxAnisotropy = 16;
    float AnisotropyThreshold = 0;
    std::uint32_t AnisotropyBias = 0;
    std::uint32_t ComparisonFunction = 1;
    std::array<float, 4> BorderColor{};
    std::uint32_t PerformanceMip = 0;
    std::uint32_t PerformanceZ = 0;
    std::array<std::uint32_t, 4> BorderColorSwizzle{};
};
using SamplerWords = std::array<std::uint32_t, 4>;
std::uint32_t GetSamplerBorderColorType(const SamplerDescription& description) noexcept;
SamplerWords EncodeSampler(const SamplerDescription& description, std::uint32_t borderIndex = 0);
static_assert(sizeof(SamplerDescription) == 108);
static_assert(offsetof(SamplerDescription, BorderColor) == 68);
static_assert(offsetof(SamplerDescription, BorderColorSwizzle) == 92);
}
