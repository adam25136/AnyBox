#include "TypedBuffer.hpp"
#include "TypedFormats.hpp"
#include <algorithm>
#include <stdexcept>

std::array<std::uint32_t, 8> AnyBox::Graphics::EncodeTypedBuffer(std::uint64_t address, std::uint64_t width, const D3D12_SHADER_RESOURCE_VIEW_DESC& view) {
    if (view.ViewDimension != D3D12_SRV_DIMENSION_BUFFER || view.Buffer.Flags != D3D12_BUFFER_SRV_FLAG_NONE) throw std::invalid_argument("Typed buffer view dimension or flags are invalid");
    if (view.Buffer.StructureByteStride != 0) throw std::invalid_argument("Typed buffer view cannot have a structure stride");
    auto format = std::ranges::find(TypedBufferFormats, view.Format, &TypedBufferFormat::format);
    if (format == TypedBufferFormats.end()) throw std::runtime_error("Typed buffer shader view format is not implemented");
    if (view.Buffer.FirstElement > width / format->size) throw std::out_of_range("Typed buffer view first element exceeds its resource");
    auto offset = view.Buffer.FirstElement * format->size;
    auto available = (width - offset) / format->size;
    auto count = view.Buffer.NumElements == UINT32_MAX ? available : view.Buffer.NumElements;
    if (count > UINT32_MAX || count > available) throw std::out_of_range("Typed buffer view exceeds its resource extent");
    if (address > 0xFFFFFFFFFFFFULL || offset > 0xFFFFFFFFFFFFULL - address) throw std::invalid_argument("Typed buffer view address cannot be encoded");
    address += offset;
    std::uint32_t mapping = 0;
    for (unsigned component = 0; component < 4; ++component) {
        auto selector = (view.Shader4ComponentMapping >> (component * 3)) & 7;
        if (selector > 5) throw std::invalid_argument("Typed buffer view component selector is invalid");
        auto value = selector < 4 ? format->components[selector] : selector - 4;
        mapping |= value << (component * 3);
    }
    return {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32) | (format->size << 16), static_cast<std::uint32_t>(count), 0x10000000 | (format->encoding << 12) | mapping, 0, 0, 0, 0};
}
