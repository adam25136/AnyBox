#pragma once

#include <cstdint>

namespace AnyBox::Graphics {
void ValidateDescriptorRange(std::uint64_t address, std::uint32_t count, std::uint32_t stride, bool sampler);
void CopyDescriptorRange(std::uint64_t destination, std::uint64_t source, std::uint32_t count, std::uint32_t stride, bool sampler);
}
