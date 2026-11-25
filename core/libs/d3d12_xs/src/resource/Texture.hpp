#pragma once

#include "../device/Device.hpp"
#include <anybox/texture/Resource.hpp>
#include <anybox/descriptor/ColorTarget.hpp>
#include "ResourceMemory.hpp"

namespace AnyBox::Graphics {
struct TextureBacking {
    TextureBacking(Device& device, ResourceMemory memory, const Vulkan::ImageDescription& image, const Descriptor::Texture2DDescription& description, const D3D12_HEAP_PROPERTIES& properties, D3D12_HEAP_FLAGS flags, D3D12_BARRIER_LAYOUT layout, const Descriptor::SurfacePitch& pitch);
    ResourceMemory memory;
    Texture::Resource allocation;
    D3D12_HEAP_PROPERTIES properties;
    D3D12_HEAP_FLAGS flags;
    D3D12_BARRIER_LAYOUT initialLayout;
    Descriptor::SurfacePitch pitch;
};

std::unique_ptr<TextureBacking> CreateTextureBacking(Device* device, const D3D12_HEAP_PROPERTIES& properties, D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC1& description, D3D12_BARRIER_LAYOUT layout, const D3D12_CLEAR_VALUE* clearValue);
struct TextureFormatDescription {
    VkFormat format;
    std::uint32_t hardwareFormat;
};
TextureFormatDescription TextureFormat(DXGI_FORMAT format);
}
