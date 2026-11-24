#pragma once

#include "../device/Device.hpp"
#include "../object/Lifetime.hpp"
#include <anybox/vulkan/CommandArena.hpp>

namespace AnyBox::Graphics {
inline constexpr Guid AllocatorId{0x6102DEE4, 0xAF59, 0x4B09, {0xB9, 0x99, 0xB4, 0x4D, 0x73, 0xF0, 0x9B, 0x24}};
const ObjectTable& GetAllocatorTable();

class Allocator : public ObjectLifetime<Allocator> {
public:
    Allocator(ID3D12CommandAllocator* host, Device* device, D3D12_COMMAND_LIST_TYPE type);
    ~Allocator();
    static constexpr std::size_t GetVtableOffset() { return offsetof(Allocator, vtable); }
    ID3D12CommandAllocator* GetHost() const noexcept { return host; }
    Device* GetParent() const noexcept { return device; }
    D3D12_COMMAND_LIST_TYPE GetType() const noexcept { return type; }
    Vulkan::CommandArena& GetGpuArena() {
        std::scoped_lock lock(gpuMutex);
        if (!gpu) gpu = new Vulkan::CommandArena(device->GetGpuContext());
        return *gpu;
    }
    static Allocator* From(void* object);
    static bool Supports(const Guid& interfaceId) noexcept;
    static HRESULT Reset(Allocator* self);

private:
    friend class ObjectLifetime<Allocator>;
    const ObjectTable* vtable;
    std::atomic<std::uint32_t>& getReferences() noexcept { return references; }
    std::atomic<std::uint32_t> references{1};
    ID3D12CommandAllocator* host;
    Device* device;
    D3D12_COMMAND_LIST_TYPE type;
    std::mutex gpuMutex;
    Vulkan::CommandArena* gpu = nullptr;
};

static_assert(std::is_standard_layout_v<Allocator>);
static_assert(Allocator::GetVtableOffset() == 0);
}
