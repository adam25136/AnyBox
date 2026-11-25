#include "SamplerStorage.hpp"
#include <cstring>
#include <stdexcept>

AnyBox::Graphics::SamplerStorage AnyBox::Graphics::ReadSamplerStorage(std::uint64_t address, std::uint32_t count) {
    if (address == 0 || address % 16 != 0) throw std::invalid_argument("Xbox sampler handle is null or misaligned");
    auto base = address & ~std::uint64_t{65535};
    auto offset = address - base;
    if (offset < 16) throw std::invalid_argument("Xbox sampler handle points to its heap prefix");
    auto index = static_cast<std::uint32_t>(offset / 16 - 1);
    std::uint32_t capacity;
    std::memcpy(&capacity, reinterpret_cast<const void*>(static_cast<std::uintptr_t>(base)), sizeof(capacity));
    if (capacity == 0 || capacity > 4095 || index >= capacity || count > capacity - index) throw std::out_of_range("Xbox sampler range is outside its heap");
    auto table = (16ULL * capacity + 271) & ~std::uint64_t{255};
    return {index, static_cast<std::uintptr_t>(base + table + 16ULL * index)};
}
