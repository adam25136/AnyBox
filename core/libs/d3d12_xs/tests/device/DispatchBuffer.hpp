#pragma once

#include "Tests.hpp"
#include "../../src/resource/Resource.hpp"
#include <xmem.hpp>

namespace AnyBox::Graphics::Tests {
class DispatchBuffer {
public:
    explicit DispatchBuffer(Device* device) {
        std::size_t count = 1;
        Require(XMemAllocatePhysicalPages(0x280, &count, &frame) == 1 && count == 1, "Dispatch output physical allocation failed");
        try {
            address = XMemVirtualAlloc(nullptr, PhysicalPageSize, 0x20402000, 0x23, 0x2004);
            Require(address != nullptr, "Dispatch output virtual allocation failed");
            XMemMapPhysicalPages(address, 1, &frame);
            D3D12_RESOURCE_DESC1 description{};
            description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            description.Width = PhysicalPageSize;
            description.Height = 1;
            description.DepthOrArraySize = 1;
            description.MipLevels = 1;
            description.SampleDesc.Count = 1;
            description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            description.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            void* output = nullptr;
            Require(Method<decltype(&Device::CreatePlacedResourceLayoutX)>(device, 133)(device, GetAddress(), &description, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, ResourceId, &output) == S_OK && output, "Dispatch output resource creation failed");
            resource = static_cast<Resource*>(output);
        } catch (...) { release(); throw; }
    }
    ~DispatchBuffer() { release(); }
    std::uintptr_t GetAddress() const noexcept { return reinterpret_cast<std::uintptr_t>(address); }
    std::uint32_t Read(std::size_t index) const { std::uint32_t value; std::memcpy(&value, static_cast<std::byte*>(address) + index * 4, 4); return value; }

private:
    void release() {
        if (resource) Method<decltype(&Resource::Release)>(resource, 2)(resource);
        if (address) {
            XMemMapPhysicalPages(address, 1, nullptr);
            Require(VirtualFree(address, 0, MEM_RELEASE) != FALSE, "Dispatch output virtual reservation release failed");
        }
        XMemFreePhysicalPages(1, &frame);
    }
    std::uint64_t frame = 0;
    void* address = nullptr;
    Resource* resource = nullptr;
};
}
