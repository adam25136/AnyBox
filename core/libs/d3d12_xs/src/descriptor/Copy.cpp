#include "Copy.hpp"
#include "SamplerStorage.hpp"
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

void AnyBox::Graphics::ValidateDescriptorRange(std::uint64_t address, std::uint32_t count, std::uint32_t stride, bool sampler) {
    if (count == 0) return;
    if (address == 0 || address % 16 != 0) throw std::invalid_argument("Xbox descriptor range is null or misaligned");
    auto bytes = static_cast<std::uint64_t>(count) * stride;
    if (bytes > std::numeric_limits<std::uintptr_t>::max() - address) throw std::overflow_error("Xbox descriptor range address overflows");
    if (sampler) ReadSamplerStorage(address, count);
}

void AnyBox::Graphics::CopyDescriptorRange(std::uint64_t destination, std::uint64_t source, std::uint32_t count, std::uint32_t stride, bool sampler) {
    if (count == 0) return;
    auto bytes = static_cast<std::uint64_t>(count) * stride;
    if (destination < source + bytes && source < destination + bytes) throw std::invalid_argument("Xbox descriptor copy ranges overlap");
    if (!sampler) {
        std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination)), reinterpret_cast<const void*>(static_cast<std::uintptr_t>(source)), static_cast<std::size_t>(bytes));
        return;
    }
    auto output = ReadSamplerStorage(destination, count);
    auto input = ReadSamplerStorage(source, count);
    std::memcpy(reinterpret_cast<void*>(output.colors), reinterpret_cast<const void*>(input.colors), static_cast<std::size_t>(count) * 16);
    for (std::uint32_t index = 0; index < count; ++index) {
        std::array<std::uint32_t, 4> words;
        std::memcpy(words.data(), reinterpret_cast<const void*>(static_cast<std::uintptr_t>(source + 16ULL * index)), sizeof(words));
        words[3] = (words[3] & ~0xFFFu) | (output.index + index);
        std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination + 16ULL * index)), words.data(), sizeof(words));
    }
}
