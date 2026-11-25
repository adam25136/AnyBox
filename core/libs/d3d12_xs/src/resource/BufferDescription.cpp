#include "Backing.hpp"
#include <anybox/Unimplemented.hpp>
#include <limits>

void AnyBox::Graphics::ValidateBufferDescription(const D3D12_RESOURCE_DESC1& description) {
    if (description.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) ANYBOX_NOT_IMPLEMENTED();
    if (description.Width == 0 || description.Width > std::numeric_limits<std::size_t>::max() - (PhysicalPageSize - 1)) throw std::invalid_argument("Buffer width is invalid");
    if (description.Height != 1 || description.DepthOrArraySize != 1 || description.MipLevels != 1 || description.Format != DXGI_FORMAT_UNKNOWN || description.SampleDesc.Count != 1 || description.SampleDesc.Quality != 0 || description.Layout != D3D12_TEXTURE_LAYOUT_ROW_MAJOR) throw std::invalid_argument("Buffer description is invalid");
    if (description.Alignment != 0 && description.Alignment != PhysicalPageSize) ANYBOX_NOT_IMPLEMENTED();
    if (description.SamplerFeedbackMipRegion.Width != 0 || description.SamplerFeedbackMipRegion.Height != 0 || description.SamplerFeedbackMipRegion.Depth != 0) throw std::invalid_argument("Buffer cannot have a sampler feedback mip region");
    constexpr auto allowedFlags = static_cast<std::uint32_t>(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) | 0x20000u;
    if ((static_cast<std::uint32_t>(description.Flags) & ~allowedFlags) != 0) ANYBOX_NOT_IMPLEMENTED();
}
