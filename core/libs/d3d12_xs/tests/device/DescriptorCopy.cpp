#include "DescriptorCopyFixture.hpp"

void AnyBox::Graphics::Tests::CheckDescriptorCopies(Device* device) {
    auto copy = Method<decltype(&Device::CopyDescriptorsSimple)>(device, 24);
    for (auto type : {0u, 2u, 3u, 4u}) {
        auto heapType = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(type);
        auto source = MakeCopyHeap(device, heapType);
        auto destination = MakeCopyHeap(device, heapType, type == 0);
        auto input = source->GetGpuBuffer().Map();
        auto output = destination->GetGpuBuffer().Map();
        for (std::size_t index = 0; index < input.size(); ++index) input[index] = static_cast<std::byte>((index * 13 + index / 7) & 255);
        auto stride = Method<decltype(&Device::GetDescriptorHandleIncrementSize)>(device, 15)(device, heapType);
        auto inputAddress = CopyHeapStart(source.get());
        auto outputAddress = CopyHeapStart(destination.get());
        for (auto count : {0u, 1u, 3u, 10u}) {
            std::ranges::fill(output, std::byte{0xCC});
            auto expected = std::vector<std::byte>(output.begin(), output.end());
            std::memcpy(expected.data() + 3 * stride, input.data() + 2 * stride, count * stride);
            copy(device, count, outputAddress + 3 * stride, inputAddress + 2 * stride, heapType);
            Require(std::ranges::equal(output, expected), "Descriptor copy changed a neighboring entry or the descriptor stride");
        }
        CheckCopiedGpuMemory(device, destination.get());
        copy(device, 0, 0, 0, heapType);
        auto snapshot = std::vector<std::byte>(output.begin(), output.end());
        Reject<std::invalid_argument>([&] { copy(device, 1, 0, inputAddress, heapType); });
        Reject<std::invalid_argument>([&] { copy(device, 1, outputAddress + 1, inputAddress, heapType); });
        Reject<std::invalid_argument>([&] { copy(device, 2, outputAddress, outputAddress + stride, heapType); });
        Reject<std::overflow_error>([&] { copy(device, 1, UINT64_MAX - 15, inputAddress, heapType); });
        Require(std::ranges::equal(output, snapshot), "Rejected descriptor copy changed its destination");
    }
    Reject<std::invalid_argument>([&] { copy(device, 0, 0, 0, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(5)); });
    CheckSamplerCopies(device);
    CheckDescriptorRanges(device);
}
