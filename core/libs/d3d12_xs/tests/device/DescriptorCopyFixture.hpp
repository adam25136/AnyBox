#pragma once

#include "Tests.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>
#include <memory>

namespace AnyBox::Graphics::Tests {
struct DescriptorCopyDeleter {
    void operator()(DescriptorHeap* value) const { Method<decltype(&DescriptorHeap::Release)>(value, 2)(value); }
};
using OwnedDescriptorCopyHeap = std::unique_ptr<DescriptorHeap, DescriptorCopyDeleter>;
inline OwnedDescriptorCopyHeap MakeCopyHeap(Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, bool visible = false) {
    D3D12_DESCRIPTOR_HEAP_DESC description{type, 16, visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateDescriptorHeap)>(device, 14)(device, &description, DescriptorHeapId, &output) == S_OK && output, "Descriptor copy heap creation failed");
    return OwnedDescriptorCopyHeap(static_cast<DescriptorHeap*>(output));
}
inline std::uint64_t CopyHeapStart(DescriptorHeap* heap) { return Method<decltype(&DescriptorHeap::GetCpuStart)>(heap, 9)(heap); }
inline void CheckCopiedGpuMemory(Device* device, DescriptorHeap* heap) {
    auto memory = heap->GetGpuBuffer().Map();
    auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), memory.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Vulkan::Tests::Copy(device->GetGpuContext(), heap->GetGpuBuffer(), scratch, memory.size());
    Require(std::ranges::equal(memory, scratch.Map()), "GPU did not observe copied descriptors");
}
void CheckSamplerCopies(Device* device);
void CheckDescriptorRanges(Device* device);
}
