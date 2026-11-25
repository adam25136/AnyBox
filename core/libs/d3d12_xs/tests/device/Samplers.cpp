#include "Tests.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/descriptor/Sampler.hpp>
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckSamplers(Device* device) {
    auto create = Method<decltype(&Device::CreateSamplerX)>(device, 61);
    for (bool visible : {false, true}) {
        D3D12_DESCRIPTOR_HEAP_DESC description{D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 256, visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0};
        void* output = nullptr;
        Require(Method<decltype(&Device::CreateDescriptorHeap)>(device, 14)(device, &description, DescriptorHeapId, &output) == S_OK && output, "Sampler heap creation failed");
        auto* heap = static_cast<DescriptorHeap*>(output);
        auto start = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap, 9)(heap);
        auto memory = heap->GetGpuBuffer().Map();
        auto colorOffset = (16 * description.NumDescriptors + 271) & ~std::uint32_t{255};
        Descriptor::SamplerDescription sampler;
        for (auto index : {0u, 3u, 255u}) {
            auto destination = start + 16 * index;
            for (const auto& color : std::array<std::array<float, 4>, 4>{{{0, 0, 0, 0}, {0, 0, 0, 1}, {1, 1, 1, 1}, {0.25f, 0.5f, 0.75f, 1}}}) {
                sampler.BorderColor = color;
                create(device, &sampler, destination);
                auto expected = Descriptor::EncodeSampler(sampler, index);
                Require(std::memcmp(reinterpret_cast<const void*>(destination), expected.data(), sizeof(expected)) == 0, "Sampler descriptor changed across the Xbox ABI");
                if (Descriptor::GetSamplerBorderColorType(sampler) == 3) Require(std::memcmp(memory.data() + colorOffset + 16 * index, color.data(), sizeof(color)) == 0, "Sampler custom border color was not stored in its heap table");
            }
        }
        auto snapshot = std::vector<std::byte>(memory.begin(), memory.end());
        sampler.Flags = 64;
        Reject<std::runtime_error>([&] { create(device, &sampler, start); });
        Require(std::ranges::equal(snapshot, memory), "Rejected sampler changed its descriptor or border color");
        sampler.Flags = 0;
        Reject<std::out_of_range>([&] { create(device, &sampler, start + 16 * description.NumDescriptors); });
        Reject<std::invalid_argument>([&] { create(device, &sampler, 0); });
        Reject<std::invalid_argument>([&] { create(device, &sampler, start + 1); });
        create(device, nullptr, start);
        auto defaults = Descriptor::EncodeSampler({});
        Require(std::memcmp(reinterpret_cast<const void*>(start), defaults.data(), sizeof(defaults)) == 0, "Null sampler description did not create the original default sampler");
        std::uint32_t count = 0;
        std::memcpy(&count, memory.data(), sizeof(count));
        Require(count == description.NumDescriptors, "Sampler creation corrupted its heap count prefix");
        auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), memory.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        Vulkan::Tests::Copy(device->GetGpuContext(), heap->GetGpuBuffer(), scratch, memory.size());
        Require(std::ranges::equal(memory, scratch.Map()), "GPU did not observe the sampler descriptors and custom border colors");
        Require(Method<decltype(&DescriptorHeap::Release)>(heap, 2)(heap) == 0, "Sampler heap was not released");
    }
}
