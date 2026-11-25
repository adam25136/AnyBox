#include "DescriptorCopyFixture.hpp"

void AnyBox::Graphics::Tests::CheckDescriptorRanges(Device* device) {
    auto copy = Method<decltype(&Device::CopyDescriptors)>(device, 23);
    auto source = MakeCopyHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto destination = MakeCopyHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
    auto input = source->GetGpuBuffer().Map();
    auto output = destination->GetGpuBuffer().Map();
    for (std::size_t index = 0; index < input.size(); ++index) input[index] = static_cast<std::byte>(index * 17 + index / 11);
    std::ranges::fill(output, std::byte{0xCC});
    auto inputAddress = CopyHeapStart(source.get());
    auto outputAddress = CopyHeapStart(destination.get());
    std::array<std::uint64_t, 4> destinations{0, outputAddress + 64, outputAddress + 288, outputAddress + 480};
    std::array<std::uint32_t, 4> destinationSizes{0, 5, 4, 1};
    std::array<std::uint64_t, 4> sources{inputAddress + 32, 0, inputAddress + 256, inputAddress + 448};
    std::array<std::uint32_t, 4> sourceSizes{4, 0, 5, 1};
    constexpr std::array<unsigned, 10> inputIndices{1, 2, 3, 4, 8, 9, 10, 11, 12, 14};
    constexpr std::array<unsigned, 10> outputIndices{2, 3, 4, 5, 6, 9, 10, 11, 12, 15};
    auto expected = std::vector<std::byte>(output.begin(), output.end());
    for (unsigned index = 0; index < inputIndices.size(); ++index) std::memcpy(expected.data() + outputIndices[index] * 32, input.data() + inputIndices[index] * 32, 32);
    copy(device, 4, destinations.data(), destinationSizes.data(), 4, sources.data(), sourceSizes.data(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    Require(std::ranges::equal(output, expected), "Descriptor ranges did not preserve their flattened copy order");
    destinations = {outputAddress, outputAddress + 256, 0, 0};
    sources = {inputAddress + 96, inputAddress + 288, 0, 0};
    copy(device, 2, destinations.data(), nullptr, 2, sources.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    std::memcpy(expected.data(), input.data() + 96, 32);
    std::memcpy(expected.data() + 256, input.data() + 288, 32);
    Require(std::ranges::equal(output, expected), "Descriptor ranges did not apply implicit single-descriptor sizes");
    CheckCopiedGpuMemory(device, destination.get());
    copy(device, 0, nullptr, nullptr, 0, nullptr, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    Reject<std::invalid_argument>([&] { copy(device, 1, nullptr, nullptr, 0, nullptr, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); });
    Reject<std::invalid_argument>([&] { copy(device, 2, destinations.data(), nullptr, 1, sources.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); });
    destinations[1] = 0;
    Reject<std::invalid_argument>([&] { copy(device, 2, destinations.data(), nullptr, 2, sources.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); });
    Require(std::ranges::equal(output, expected), "Rejected descriptor ranges changed a preceding valid destination");
}
