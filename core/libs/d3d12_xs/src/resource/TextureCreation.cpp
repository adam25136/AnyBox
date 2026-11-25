#include "Texture.hpp"
#include "TextureAllocation.hpp"

namespace {
void CheckLayout(const D3D12_RESOURCE_DESC1& description, D3D12_BARRIER_LAYOUT layout) {
    auto flags = description.Flags;
    if (layout == D3D12_BARRIER_LAYOUT_UNDEFINED || layout == D3D12_BARRIER_LAYOUT_COMMON || layout == D3D12_BARRIER_LAYOUT_COPY_SOURCE || layout == D3D12_BARRIER_LAYOUT_COPY_DEST) return;
    if (layout == D3D12_BARRIER_LAYOUT_RENDER_TARGET && (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0) return;
    if (layout == D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS && (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0) return;
    if (layout == D3D12_BARRIER_LAYOUT_SHADER_RESOURCE && (flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0) return;
    throw std::runtime_error("Committed texture initial layout is unsupported for its resource flags");
}
}

std::unique_ptr<AnyBox::Graphics::TextureBacking> AnyBox::Graphics::CreateTextureBacking(Device* device, const D3D12_HEAP_PROPERTIES& properties, D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC1& description, D3D12_BARRIER_LAYOUT layout, const D3D12_CLEAR_VALUE* clearValue) {
    if (properties.Type != D3D12_HEAP_TYPE_DEFAULT || properties.CPUPageProperty != D3D12_CPU_PAGE_PROPERTY_UNKNOWN || properties.MemoryPoolPreference != D3D12_MEMORY_POOL_UNKNOWN) throw std::runtime_error("Committed texture heap type is not implemented");
    if (properties.CreationNodeMask > 1 || properties.VisibleNodeMask > 1) throw std::invalid_argument("Committed texture node mask is invalid");
    if ((static_cast<std::uint32_t>(flags) & ~static_cast<std::uint32_t>(D3D12_HEAP_FLAG_ALLOW_DISPLAY)) != 0) throw std::runtime_error("Committed texture heap flags are not implemented");
    if (description.SamplerFeedbackMipRegion.Width != 0 || description.SamplerFeedbackMipRegion.Height != 0 || description.SamplerFeedbackMipRegion.Depth != 0) throw std::runtime_error("Committed texture sampler feedback is not implemented");
    if (description.Layout != D3D12_TEXTURE_LAYOUT_UNKNOWN) throw std::runtime_error("Committed texture explicit layout is not implemented");
    D3D12_RESOURCE_DESC base{};
    std::memcpy(&base, &description, offsetof(D3D12_RESOURCE_DESC, Flags) + sizeof(base.Flags));
    auto allocation = ComputeTextureSurface(base);
    CheckLayout(description, layout);
    if (clearValue && ((description.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == 0 || clearValue->Format != description.Format)) throw std::invalid_argument("Committed texture clear value is incompatible with its description");

    Vulkan::ImageDescription image;
    auto format = TextureFormat(description.Format);
    image.format = format.format;
    image.extent = {static_cast<std::uint32_t>(description.Width), description.Height, 1};
    image.mipLevels = description.MipLevels;
    image.arrayLayers = description.DepthOrArraySize;
    image.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if ((description.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0) image.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if ((description.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0) image.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ((description.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0) image.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    Descriptor::Texture2DDescription guest{image.extent.width, image.extent.height, image.arrayLayers, image.mipLevels, format.hardwareFormat, ComputeTextureSwizzleMode(base)};
    ResourceMemory memory(allocation.size, allocation.alignment, flags);
    Descriptor::SurfacePitch pitch{allocation.pixelPitch, allocation.pixelHeight};
    return std::make_unique<TextureBacking>(*device, std::move(memory), image, guest, properties, flags, layout, pitch);
}
