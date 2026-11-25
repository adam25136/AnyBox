#pragma once

#include <d3d12.h>
#include <array>
#include <cstdint>

namespace AnyBox::Graphics {
std::array<std::uint32_t, 8> EncodeTypedBuffer(std::uint64_t address, std::uint64_t width, const D3D12_SHADER_RESOURCE_VIEW_DESC& view);
}
