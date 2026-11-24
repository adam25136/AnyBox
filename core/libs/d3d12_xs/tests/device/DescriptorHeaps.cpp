#include "Tests.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckDescriptorHeaps(Device* device) {
    auto create = Method<decltype(&Device::CreateDescriptorHeap)>(device, 14);
    for (std::uint32_t type = 0; type < 5; ++type) {
        for (bool visible : {false, true}) {
            D3D12_DESCRIPTOR_HEAP_DESC description{static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(type), 33, visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1};
            void* output = nullptr;
            Require(create(device, &description, DescriptorHeapId, &output) == S_OK && output, "Xbox descriptor heap creation failed");
            auto* heap = static_cast<DescriptorHeap*>(output);
            auto release = Method<decltype(&DescriptorHeap::Release)>(heap, 2);
            auto query = Method<decltype(&DescriptorHeap::QueryInterface)>(heap, 0);
            for (const auto& id : {DescriptorHeapId, UnknownId, ObjectId, DeviceChildId, PageableId}) {
                void* identity = nullptr;
                Require(query(heap, id, &identity) == S_OK && identity == heap && release(heap) == 1, "Descriptor heap COM identity changed");
            }
            D3D12_DESCRIPTOR_HEAP_DESC observed{};
            auto getDescription = Method<decltype(&DescriptorHeap::GetDescription)>(heap, 8);
            getDescription(heap, &observed);
            Require(observed.Type == description.Type && observed.NumDescriptors == description.NumDescriptors && observed.Flags == description.Flags && observed.NodeMask == 0, "Xbox descriptor heap description layout changed");
            Reject<std::invalid_argument>([&] { getDescription(heap, nullptr); });
            auto cpu = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap, 9)(heap);
            auto gpu = Method<decltype(&DescriptorHeap::GetGpuStart)>(heap, 10)(heap);
            auto& buffer = heap->GetGpuBuffer();
            auto memory = buffer.Map();
            auto expected = reinterpret_cast<std::uintptr_t>(memory.data()) + (type == 1 ? 16 : 0);
            Require(cpu == expected && gpu == expected, "Xbox descriptor handles do not refer to actual descriptor storage");
            if (type == 1) {
                std::uint32_t count = 0;
                std::memcpy(&count, memory.data(), sizeof(count));
                Require(count == description.NumDescriptors, "Xbox sampler heap has no descriptor count prefix");
            }
            constexpr auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), memory.size(), usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            Vulkan::Tests::CheckCompute(device->GetGpuContext(), buffer, scratch, false);
            auto& addresses = device->GetDescriptorAddresses();
            std::shared_ptr<const Buffers::Mapping> retained;
            if (type == 0 && visible) {
                retained = addresses.Resolve(cpu, description.NumDescriptors * 32);
                Require(retained->Translate(cpu + 32, 32) == buffer.GetAddress() + 32, "Shader-visible descriptor heap address translation changed");
                Reject<std::out_of_range>([&] { (void)addresses.Resolve(cpu + description.NumDescriptors * 32, 32); });
            } else Reject<std::out_of_range>([&] { (void)addresses.Resolve(cpu); });
            Require(release(heap) == 0, "Descriptor heap final reference was not released");
            if (retained) {
                Vulkan::Tests::Copy(device->GetGpuContext(), buffer, scratch, memory.size());
                Require(std::ranges::equal(memory, scratch.Map()), "Retained descriptor storage was freed before GPU execution");
                retained.reset();
                Require(addresses.Capture().mappings.empty(), "Released descriptor heap retained a reference cycle");
            }
        }
    }
    D3D12_DESCRIPTOR_HEAP_DESC description{};
    void* output = nullptr;
    for (auto type : {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER}) {
        description.Type = type;
        Require(create(device, &description, DescriptorHeapId, &output) == S_OK && output, "Empty Xbox descriptor heap creation failed");
        auto* heap = static_cast<DescriptorHeap*>(output);
        auto cpu = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap, 9)(heap);
        if (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) Require(cpu != 0 && heap->GetGpuBuffer().GetSize() == 256, "Empty sampler heap lost its prefix storage");
        else Require(cpu == 0, "Empty view heap has a nonzero descriptor address");
        Require(Method<decltype(&DescriptorHeap::Release)>(heap, 2)(heap) == 0, "Empty descriptor heap was not released");
    }
    Reject<std::invalid_argument>([&] { create(device, nullptr, DescriptorHeapId, &output); });
    Reject<std::invalid_argument>([&] { create(device, &description, UnknownId, &output); });
    Reject<std::invalid_argument>([&] { create(device, &description, DescriptorHeapId, nullptr); });
    description.NodeMask = 2;
    Reject<std::runtime_error>([&] { create(device, &description, DescriptorHeapId, &output); });
    description.NodeMask = 0;
    description.Flags = static_cast<D3D12_DESCRIPTOR_HEAP_FLAGS>(2);
    Reject<std::runtime_error>([&] { create(device, &description, DescriptorHeapId, &output); });
    description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    description.NumDescriptors = UINT32_MAX;
    Reject<std::length_error>([&] { create(device, &description, DescriptorHeapId, &output); });
    description.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(5);
    Reject<std::invalid_argument>([&] { create(device, &description, DescriptorHeapId, &output); });
    Require(!output, "Rejected descriptor heap creation retained an output interface");
}
