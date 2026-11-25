#include "Backing.hpp"
#include <anybox/Unimplemented.hpp>
#include <limits>

AnyBox::Graphics::BufferMemory::BufferMemory(Device* device) : device(device) { Device::AddRef(device); }

AnyBox::Graphics::BufferMemory::~BufferMemory() {
    resource.reset();
    pages.reset();
    allocation.reset();
    Device::Release(device);
}

AnyBox::Graphics::BufferBacking AnyBox::Graphics::CreateBufferBacking(Device* device, std::uintptr_t address, const D3D12_RESOURCE_DESC1& description) {
    static_assert(sizeof(D3D12_RESOURCE_DESC1) == 64);
    ValidateBufferDescription(description);
    if (address == 0 || address % PhysicalPageSize != 0) throw std::invalid_argument("Placed buffer address is invalid");

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
