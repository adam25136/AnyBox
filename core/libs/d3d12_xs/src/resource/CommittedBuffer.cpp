#include "Backing.hpp"
#include <algorithm>
#include <limits>
#include <utility>

namespace {
void CheckHeap(const D3D12_HEAP_PROPERTIES& properties, D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC1& description, D3D12_RESOURCE_STATES state) {
    if (properties.Type != D3D12_HEAP_TYPE_DEFAULT && properties.Type != D3D12_HEAP_TYPE_UPLOAD && properties.Type != D3D12_HEAP_TYPE_READBACK) throw std::runtime_error("Committed buffer heap type is not implemented");
    if (properties.CPUPageProperty != D3D12_CPU_PAGE_PROPERTY_UNKNOWN || properties.MemoryPoolPreference != D3D12_MEMORY_POOL_UNKNOWN) throw std::invalid_argument("Committed buffer standard heap has custom memory properties");
    if (properties.CreationNodeMask > 1 || properties.VisibleNodeMask > 1) throw std::invalid_argument("Committed buffer node mask is invalid");
    constexpr auto allowed = static_cast<std::uint32_t>(D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES);
    if ((static_cast<std::uint32_t>(flags) & ~allowed) != 0) throw std::runtime_error("Committed buffer heap flags are not implemented");
    if (properties.Type == D3D12_HEAP_TYPE_READBACK && state != D3D12_RESOURCE_STATE_COPY_DEST) throw std::invalid_argument("Readback buffer initial state must be copy destination");
    if (properties.Type == D3D12_HEAP_TYPE_UPLOAD && state != D3D12_RESOURCE_STATE_GENERIC_READ) throw std::invalid_argument("Upload buffer initial state must be generic read");
    if (properties.Type != D3D12_HEAP_TYPE_DEFAULT && (description.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0) throw std::invalid_argument("CPU buffer heaps cannot allow unordered access");
    auto bits = static_cast<std::uint32_t>(state);
    constexpr auto readStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT | D3D12_RESOURCE_STATE_COPY_SOURCE;
    constexpr auto writeStates = D3D12_RESOURCE_STATE_UNORDERED_ACCESS | D3D12_RESOURCE_STATE_STREAM_OUT | D3D12_RESOURCE_STATE_COPY_DEST;
    if ((bits & ~static_cast<std::uint32_t>(readStates | writeStates)) != 0) throw std::runtime_error("Committed buffer initial resource state is not implemented");
    auto writes = bits & static_cast<std::uint32_t>(writeStates);
    if (writes != 0 && (bits != writes || (writes & (writes - 1)) != 0)) throw std::invalid_argument("Buffer write states cannot be combined with another initial state");
    if ((bits & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0 && (description.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0) throw std::invalid_argument("Buffer initial unordered access requires matching resource flags");
    if ((bits & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0 && (description.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) != 0) throw std::invalid_argument("Buffer initial shader resource state conflicts with its resource flags");
}
}

AnyBox::Graphics::BufferBacking AnyBox::Graphics::CreateCommittedBufferBacking(Device* device, const D3D12_HEAP_PROPERTIES& properties, D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC1& description, D3D12_RESOURCE_STATES state) {
    ValidateBufferDescription(description);
    CheckHeap(properties, flags, description, state);
    auto alignment = std::max<std::uint64_t>(PhysicalPageSize, device->GetGpuContext().GetHostPointerAlignment());
    if (description.Width > std::numeric_limits<std::size_t>::max() - (alignment - 1)) throw std::overflow_error("Committed buffer allocation size overflows");
    auto size = (description.Width + alignment - 1) & ~(alignment - 1);
    BufferBacking backing;
    backing.memory = std::make_shared<BufferMemory>(device);
    auto memoryType = properties.Type == D3D12_HEAP_TYPE_READBACK ? 4u : 1u;
    backing.memory->allocation = std::make_unique<ResourceMemory>(size, alignment, D3D12_HEAP_FLAG_NONE, memoryType);
    auto* address = static_cast<std::byte*>(backing.memory->allocation->GetAddress());
    constexpr auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    auto buffer = Vulkan::Buffer::Import(device->GetGpuContext(), {address, static_cast<std::size_t>(size)}, description.Width, usage);
    backing.memory->resource = std::make_unique<Vulkan::Buffer>(std::move(buffer));
    backing.memory->properties = properties;
    backing.memory->flags = flags;
    backing.mapping = device->GetBufferAddresses().Register(reinterpret_cast<std::uintptr_t>(address), description.Width, backing.memory->resource->GetAddress(), backing.memory);
    return backing;
}
