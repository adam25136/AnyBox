#include "Device.hpp"
#include "../descriptor/Copy.hpp"
#include <algorithm>

namespace {
std::uint64_t ValidateRanges(std::uint32_t count, const std::uint64_t* starts, const std::uint32_t* sizes, std::uint32_t stride, bool sampler) {
    if (count != 0 && !starts) throw std::invalid_argument("Xbox descriptor range start array is null");
    std::uint64_t total = 0;
    for (std::uint32_t index = 0; index < count; ++index) {
        auto size = sizes ? sizes[index] : 1;
        AnyBox::Graphics::ValidateDescriptorRange(starts[index], size, stride, sampler);
        total += size;
    }
    return total;
}
struct Cursor {
    const std::uint64_t* starts;
    const std::uint32_t* sizes;
    std::uint32_t range = 0;
    std::uint32_t offset = 0;
    std::uint32_t Available() {
        while (offset == (sizes ? sizes[range] : 1)) { ++range; offset = 0; }
        return (sizes ? sizes[range] : 1) - offset;
    }
    std::uint64_t Address(std::uint32_t stride) const { return starts[range] + static_cast<std::uint64_t>(offset) * stride; }
};
}

void AnyBox::Graphics::Device::CopyDescriptors(Device* self, std::uint32_t destinationCount, const std::uint64_t* destinations, const std::uint32_t* destinationSizes, std::uint32_t sourceCount, const std::uint64_t* sources, const std::uint32_t* sourceSizes, D3D12_DESCRIPTOR_HEAP_TYPE type) {
    auto stride = GetDescriptorHandleIncrementSize(self, type);
    bool sampler = type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    auto total = ValidateRanges(destinationCount, destinations, destinationSizes, stride, sampler);
    if (total != ValidateRanges(sourceCount, sources, sourceSizes, stride, sampler)) throw std::invalid_argument("Xbox source and destination descriptor counts differ");
    Cursor destination{destinations, destinationSizes};
    Cursor source{sources, sourceSizes};
    while (total != 0) {
        auto count = std::min(destination.Available(), source.Available());
        CopyDescriptorRange(destination.Address(stride), source.Address(stride), count, stride, sampler);
        destination.offset += count;
        source.offset += count;
        total -= count;
    }
}
