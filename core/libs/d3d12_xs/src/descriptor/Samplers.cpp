#include "../device/Device.hpp"
#include <anybox/descriptor/Sampler.hpp>

void AnyBox::Graphics::Device::CreateSamplerX(Device*, const Descriptor::SamplerDescription* description, std::uint64_t destination) {
    if (destination == 0 || destination % 16 != 0) throw std::invalid_argument("Xbox sampler destination is null or misaligned");
    Descriptor::SamplerDescription defaults;
    const auto& sampler = description ? *description : defaults;
    auto type = Descriptor::GetSamplerBorderColorType(sampler);
    std::uint32_t index = 0;
    std::uintptr_t colorAddress = 0;
    if (type == 3) {
        auto base = destination & ~std::uint64_t{65535};
        auto offset = destination - base;
        if (offset < 16) throw std::invalid_argument("Xbox custom border color requires a sampler heap handle");
        index = static_cast<std::uint32_t>(offset / 16 - 1);
        std::uint32_t count;
        std::memcpy(&count, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(base)), sizeof(count));
        if (count == 0 || count > 4095 || index >= count) throw std::out_of_range("Xbox custom border color sampler index is outside its heap");
        auto table = (16ULL * count + 271) & ~std::uint64_t{255};
        colorAddress = static_cast<std::uintptr_t>(base + table + 16ULL * index);
    }
    auto words = Descriptor::EncodeSampler(sampler, index);
    if (colorAddress) std::memcpy(reinterpret_cast<void*>(colorAddress), sampler.BorderColor.data(), sizeof(sampler.BorderColor));
    std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination)), words.data(), sizeof(words));
}
