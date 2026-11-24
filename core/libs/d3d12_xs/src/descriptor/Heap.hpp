#pragma once

#include "Backing.hpp"
#include "../object/Lifetime.hpp"
#include "../object/ManagedProperties.hpp"

namespace AnyBox::Graphics {
inline constexpr Guid DescriptorHeapId{0x8EFB471D, 0x616C, 0x4F49, {0x90, 0xF7, 0x12, 0x7B, 0xB7, 0x63, 0xFA, 0x51}};
const ObjectTable& GetDescriptorHeapTable();

class alignas(16) DescriptorHeap : public ObjectLifetime<DescriptorHeap, ManagedProperties> {
public:
    DescriptorHeap(DescriptorBacking backing, Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& description);
    ~DescriptorHeap();
    static bool Supports(const Guid& interfaceId) noexcept;
    static constexpr std::size_t GetVtableOffset() { return offsetof(DescriptorHeap, vtable); }
    static constexpr std::size_t GetTypeOffset() { return offsetof(DescriptorHeap, type); }
    static constexpr std::size_t GetAddressOffset() { return offsetof(DescriptorHeap, address); }
    static constexpr std::size_t GetAllocationOffset() { return offsetof(DescriptorHeap, allocation); }
    static constexpr std::size_t GetDeviceOffset() { return offsetof(DescriptorHeap, device); }
    static void GetDescription(DescriptorHeap* self, D3D12_DESCRIPTOR_HEAP_DESC* description);
    static std::uint64_t GetCpuStart(DescriptorHeap* self) noexcept;
    static std::uint64_t GetGpuStart(DescriptorHeap* self) noexcept;
    PrivateData& GetPrivateDataStore() { return privateData.Get(); }
    Vulkan::Buffer& GetGpuBuffer() const {
        if (!buffer) throw std::logic_error("Empty Xbox descriptor heap has no GPU buffer");
        return *buffer;
    }

private:
    friend class ObjectLifetime<DescriptorHeap, AnyBox::Graphics::ManagedProperties>;
    std::atomic<std::uint32_t>& getReferences() noexcept { return references; }
    std::uint64_t getStart() const noexcept;
    const ObjectTable* vtable;
    std::uint32_t objectFlags = 0;
    std::atomic<std::uint32_t> references{1};
    std::uintptr_t next = 0;
    D3D12_DESCRIPTOR_HEAP_TYPE type;
    std::uint32_t count;
    std::uint32_t strideShift;
    std::uint32_t reserved = 0;
    std::uintptr_t address;
    D3D12_DESCRIPTOR_HEAP_FLAGS flags;
    std::uint32_t reservedFlags = 0;
    void* allocation;
    std::size_t allocationSize;
    std::uint64_t attributes;
    std::uint32_t allocationFlags = 3;
    std::uint32_t reservedAllocation = 0;
    std::uintptr_t addressMirror;
    std::uint32_t heapFlags = 0;
    std::uint32_t reservedHeap = 0;
    Device* device;
    Vulkan::Buffer* buffer;
    PrivateDataSlot privateData;
    DescriptorLease* lease = nullptr;
    [[maybe_unused]] std::uint64_t reservedLease = 0;
};
static_assert(std::is_standard_layout_v<DescriptorHeap>);
static_assert(DescriptorHeap::GetVtableOffset() == 0);
static_assert(DescriptorHeap::GetTypeOffset() == 24);
static_assert(DescriptorHeap::GetAddressOffset() == 40);
static_assert(DescriptorHeap::GetAllocationOffset() == 56);
static_assert(DescriptorHeap::GetDeviceOffset() == 104);
}
