#pragma once

#include <anybox/platform/Windows.hpp>
#include <d3d12.h>
#include <cstdint>

namespace AnyBox::Graphics {
class ResourceMemory {
public:
    ResourceMemory(std::uint64_t size, std::uint64_t alignment, D3D12_HEAP_FLAGS flags, std::uint32_t memoryType = 1);
    ~ResourceMemory();
    ResourceMemory(const ResourceMemory&) = delete;
    ResourceMemory& operator=(const ResourceMemory&) = delete;
    ResourceMemory(ResourceMemory&& other) noexcept;
    ResourceMemory& operator=(ResourceMemory&& other) = delete;
    void* GetAddress() const noexcept { return address; }
    std::uint64_t GetSize() const noexcept { return size; }

private:
    void* address = nullptr;
    std::uint64_t size;
    std::uint64_t attributes;
};
}
