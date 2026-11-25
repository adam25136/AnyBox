#include <anybox/descriptor/Sampler.hpp>
#include "SamplerFixed.hpp"
#include <bit>
#include <stdexcept>

std::uint32_t AnyBox::Descriptor::GetSamplerBorderColorType(const SamplerDescription& description) noexcept {
    const auto& color = description.BorderColor;
    if (color[0] != color[1] || color[0] != color[2]) return 3;
    if (color[0] == 0 && color[3] == 0) return 0;
    if (color[0] == 0 && color[3] == 1) return 1;
    if (color[0] == 1 && color[3] == 1) return 2;
    return 3;
}

AnyBox::Descriptor::SamplerWords AnyBox::Descriptor::EncodeSampler(const SamplerDescription& description, std::uint32_t borderIndex) {
    if ((description.Flags & ~0x1BFu) != 0) throw std::runtime_error("Sampler flags are not implemented");
    if (description.MagFilter > 3 || description.MinFilter > 3 || description.MipFilter > 3 || description.ZFilter > 3 || description.ReductionMode > 3) throw std::invalid_argument("Sampler filter mode is invalid");
    if (description.AddressU < 1 || description.AddressU > 5 || description.AddressV < 1 || description.AddressV > 5 || description.AddressW < 1 || description.AddressW > 5) throw std::invalid_argument("Sampler address mode is invalid");
    if (description.MaxAnisotropy == 0 || description.MaxAnisotropy > 16 || description.AnisotropyBias > 7 || description.ComparisonFunction < 1 || description.ComparisonFunction > 8) throw std::invalid_argument("Sampler anisotropy or comparison function is invalid");
    if (description.PerformanceMip > 15 || description.PerformanceZ > 15) throw std::invalid_argument("Sampler performance hint is invalid");
    if ((description.Flags & 0x100) != 0) for (auto selector : description.BorderColorSwizzle) if (selector > 7) throw std::invalid_argument("Sampler border color selector is invalid");
    if (borderIndex > 4095) throw std::out_of_range("Sampler border color index exceeds its hardware field");

    constexpr std::array<std::uint32_t, 6> addressModes{0, 0, 1, 2, 6, 3};
    auto flags = description.Flags;
    auto anisotropy = std::bit_width(description.MaxAnisotropy) - 1;
    auto threshold = SamplerPositiveFixed(description.AnisotropyThreshold, 32, 63);
    auto word0 = addressModes[description.AddressU] | (addressModes[description.AddressV] << 3) | (addressModes[description.AddressW] << 6);
    word0 |= (anisotropy << 9) | ((description.ComparisonFunction - 1) << 12) | ((flags & 1) << 15);
    word0 |= (description.AnisotropyBias << 16) | ((flags & 4) << 17) | ((flags & 8) << 17) | (threshold << 21);
    word0 |= ((flags & 2) << 26) | ((flags & 32) << 23) | (description.ReductionMode << 29);
    auto word1 = SamplerPositiveFixed(description.MinLod, 256, 4095) | (SamplerPositiveFixed(description.MaxLod, 256, 4095) << 12);
    word1 |= (description.PerformanceMip << 24) | (description.PerformanceZ << 28);
    auto word2 = (SamplerSignedFixed(description.MipLodBias, 256, 16383) & 0x3FFF) | ((SamplerSignedFixed(description.MipLodBiasSecondary, 16, 31) & 63) << 14);
    word2 |= (description.MagFilter << 20) | (description.MinFilter << 22) | (description.ZFilter << 24) | (description.MipFilter << 26);
    word2 |= ((~flags << 24) & 0x10000000) | ((~flags << 23) & 0x40000000);
    auto type = GetSamplerBorderColorType(description);
    auto word3 = (type << 30) | (type == 3 ? borderIndex : 0);
    if ((flags & 0x100) != 0) {
        std::uint32_t mapping = 0;
        for (unsigned component = 0; component < 4; ++component) mapping |= description.BorderColorSwizzle[component] << (component * 3);
        word2 = (word2 & ~0xFFFu) | mapping;
        word3 = (word3 & ~0xFFFu) | mapping;
    }
    return {word0, word1, word2, word3};
}
