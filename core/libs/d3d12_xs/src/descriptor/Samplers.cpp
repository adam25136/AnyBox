#include "../device/Device.hpp"
#include "SamplerStorage.hpp"
#include <anybox/descriptor/Sampler.hpp>

void AnyBox::Graphics::Device::CreateSamplerX(Device*, const Descriptor::SamplerDescription* description, std::uint64_t destination) {
    if (destination == 0 || destination % 16 != 0) throw std::invalid_argument("Xbox sampler destination is null or misaligned");
    Descriptor::SamplerDescription defaults;
    const auto& sampler = description ? *description : defaults;
    auto type = Descriptor::GetSamplerBorderColorType(sampler);
    std::uint32_t index = 0;
    std::uintptr_t colorAddress = 0;
    if (type == 3) {
        auto storage = ReadSamplerStorage(destination, 1);
        index = storage.index;
        colorAddress = storage.colors;
    }
    auto words = Descriptor::EncodeSampler(sampler, index);
    if (colorAddress) std::memcpy(reinterpret_cast<void*>(colorAddress), sampler.BorderColor.data(), sizeof(sampler.BorderColor));
    std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination)), words.data(), sizeof(words));
}
