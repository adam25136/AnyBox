#pragma once

#include "../device/Device.hpp"
#include "../host/Com.hpp"
#include <anybox/PhysicalMemory.hpp>
#include <anybox/vulkan/Buffer.hpp>

namespace AnyBox::Graphics {
struct BufferMemory {
    explicit BufferMemory(Device* device);
    ~BufferMemory();
    BufferMemory(const BufferMemory&) = delete;
    BufferMemory& operator=(const BufferMemory&) = delete;
    Device* device;
    ComOwner<IPhysicalPageLease> pages;
    std::unique_ptr<Vulkan::Buffer> resource;
};
struct BufferBacking {
    std::shared_ptr<BufferMemory> memory;
    std::shared_ptr<const Buffers::Mapping> mapping;
};
BufferBacking CreateBufferBacking(Device* device, std::uintptr_t address, const D3D12_RESOURCE_DESC1& description);
}
