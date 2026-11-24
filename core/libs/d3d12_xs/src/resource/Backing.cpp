#include "Backing.hpp"
#include <anybox/Unimplemented.hpp>
#include <limits>

AnyBox::Graphics::BufferMemory::BufferMemory(Device* device) : device(device) { Device::AddRef(device); }

AnyBox::Graphics::BufferMemory::~BufferMemory() {
    resource.reset();
    pages.reset();
    Device::Release(device);
}

AnyBox::Graphics::BufferBacking AnyBox::Graphics::CreateBufferBacking(Device* device, std::uintptr_t address, const D3D12_RESOURCE_DESC1& description) {
    static_assert(sizeof(D3D12_RESOURCE_DESC1) == 64);
    if (description.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) ANYBOX_NOT_IMPLEMENTED();
    if (address == 0 || address % PhysicalPageSize != 0 || description.Width == 0 || description.Width > std::numeric_limits<std::size_t>::max() - (PhysicalPageSize - 1)) throw std::invalid_argument("Placed buffer address or width is invalid");
    if (description.Height != 1 || description.DepthOrArraySize != 1 || description.MipLevels != 1 || description.Format != DXGI_FORMAT_UNKNOWN || description.SampleDesc.Count != 1 || description.SampleDesc.Quality != 0 || description.Layout != D3D12_TEXTURE_LAYOUT_ROW_MAJOR) throw std::invalid_argument("Placed buffer description is invalid");
    if (description.Alignment != 0 && description.Alignment != PhysicalPageSize) ANYBOX_NOT_IMPLEMENTED();
    if (description.SamplerFeedbackMipRegion.Width != 0 || description.SamplerFeedbackMipRegion.Height != 0 || description.SamplerFeedbackMipRegion.Depth != 0) throw std::invalid_argument("Placed buffer cannot have a sampler feedback mip region");
    constexpr auto allowedFlags = static_cast<std::uint32_t>(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) | 0x20000u;
    if ((static_cast<std::uint32_t>(description.Flags) & ~allowedFlags) != 0) ANYBOX_NOT_IMPLEMENTED();

    auto count = static_cast<std::size_t>((description.Width + PhysicalPageSize - 1) / PhysicalPageSize);
    BufferBacking backing;
    backing.memory = std::make_shared<BufferMemory>(device);
    backing.memory->pages.reset(AnyBoxAcquireMappedPhysicalPages(reinterpret_cast<void*>(address), count));
    std::span memory(reinterpret_cast<std::byte*>(address), count * PhysicalPageSize);
    constexpr auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    auto buffer = Vulkan::Buffer::Import(device->GetGpuContext(), memory, description.Width, usage);
    backing.memory->resource = std::make_unique<Vulkan::Buffer>(std::move(buffer));
    backing.mapping = device->GetBufferAddresses().Register(address, description.Width, backing.memory->resource->GetAddress(), backing.memory);
    return backing;
}
