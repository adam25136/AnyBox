#include "DescriptorCopyFixture.hpp"
#include <anybox/descriptor/Sampler.hpp>

void AnyBox::Graphics::Tests::CheckSamplerCopies(Device* device) {
    auto source = MakeCopyHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    auto destination = MakeCopyHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true);
    auto input = source->GetGpuBuffer().Map();
    auto output = destination->GetGpuBuffer().Map();
    std::ranges::fill(input.subspan(16), std::byte{});
    std::ranges::fill(output.subspan(16), std::byte{0xCC});
    auto inputAddress = CopyHeapStart(source.get());
    auto outputAddress = CopyHeapStart(destination.get());
    auto create = Method<decltype(&Device::CreateSamplerX)>(device, 61);
    auto copy = Method<decltype(&Device::CopyDescriptorsSimple)>(device, 24);
    constexpr std::array<std::array<float, 4>, 4> colors{{{0, 0, 0, 0}, {0, 0, 0, 1}, {1, 1, 1, 1}, {0.25f, 0.5f, 0.75f, 1}}};
    Descriptor::SamplerDescription sampler;
    for (unsigned index = 0; index < colors.size(); ++index) {
        sampler.BorderColor = colors[index];
        create(device, &sampler, inputAddress + 16 * (index + 1));
    }
    auto expected = std::vector<std::byte>(output.begin(), output.end());
    constexpr std::size_t table = (16 * 16 + 271) & ~std::size_t{255};
    for (unsigned index = 0; index < colors.size(); ++index) {
        std::array<std::uint32_t, 4> words;
        std::memcpy(words.data(), input.data() + 16 * (index + 2), sizeof(words));
        words[3] = (words[3] & ~0xFFFu) | (index + 7);
        std::memcpy(expected.data() + 16 * (index + 8), words.data(), sizeof(words));
        std::memcpy(expected.data() + table + 16 * (index + 7), input.data() + table + 16 * (index + 1), 16);
    }
    copy(device, 4, outputAddress + 16 * 7, inputAddress + 16, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    Require(std::ranges::equal(output, expected), "Sampler copy did not relocate its border color table and descriptor indices");
    auto ranges = Method<decltype(&Device::CopyDescriptors)>(device, 23);
    std::array<std::uint64_t, 3> destinations{outputAddress, 0, outputAddress + 16 * 12};
    std::array<std::uint32_t, 3> destinationSizes{1, 0, 3};
    std::array<std::uint64_t, 2> sources{inputAddress + 16, inputAddress + 16 * 3};
    std::array<std::uint32_t, 2> sourceSizes{2, 2};
    ranges(device, 3, destinations.data(), destinationSizes.data(), 2, sources.data(), sourceSizes.data(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    constexpr std::array<unsigned, 4> destinationIndices{0, 12, 13, 14};
    for (unsigned index = 0; index < destinationIndices.size(); ++index) {
        auto destinationIndex = destinationIndices[index];
        std::array<std::uint32_t, 4> words;
        std::memcpy(words.data(), input.data() + 16 * (index + 2), sizeof(words));
        words[3] = (words[3] & ~0xFFFu) | destinationIndex;
        std::memcpy(expected.data() + 16 * (destinationIndex + 1), words.data(), sizeof(words));
        std::memcpy(expected.data() + table + 16 * destinationIndex, input.data() + table + 16 * (index + 1), 16);
    }
    Require(std::ranges::equal(output, expected), "Sampler descriptor ranges changed their border colors or destination indices");
    CheckCopiedGpuMemory(device, destination.get());
    auto snapshot = expected;
    Reject<std::out_of_range>([&] { copy(device, 2, outputAddress + 16 * 15, inputAddress, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER); });
    Reject<std::out_of_range>([&] { copy(device, 2, outputAddress, inputAddress + 16 * 15, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER); });
    Reject<std::invalid_argument>([&] { copy(device, 1, outputAddress - 16, inputAddress, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER); });
    Require(std::ranges::equal(output, snapshot), "Rejected sampler copy changed its heap");
}
