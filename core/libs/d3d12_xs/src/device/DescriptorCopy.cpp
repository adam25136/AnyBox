#include "Device.hpp"
#include "../descriptor/Copy.hpp"

void AnyBox::Graphics::Device::CopyDescriptorsSimple(Device* self, std::uint32_t count, std::uint64_t destination, std::uint64_t source, D3D12_DESCRIPTOR_HEAP_TYPE type) {
    auto stride = GetDescriptorHandleIncrementSize(self, type);
    bool sampler = type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    ValidateDescriptorRange(destination, count, stride, sampler);
    ValidateDescriptorRange(source, count, stride, sampler);
    CopyDescriptorRange(destination, source, count, stride, sampler);
}
