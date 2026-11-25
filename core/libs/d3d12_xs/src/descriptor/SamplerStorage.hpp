#pragma once

#include <cstdint>

namespace AnyBox::Graphics {
struct SamplerStorage {
    std::uint32_t index;
    std::uintptr_t colors;
};
SamplerStorage ReadSamplerStorage(std::uint64_t address, std::uint32_t count);
}
