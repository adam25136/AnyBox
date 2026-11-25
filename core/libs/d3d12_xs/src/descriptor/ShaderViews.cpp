#include "../device/Device.hpp"
#include "../resource/Resource.hpp"
#include "RawBuffer.hpp"
#include "TypedBuffer.hpp"
#include "TextureViews.hpp"
#include <anybox/Unimplemented.hpp>

void AnyBox::Graphics::Device::CreateShaderResourceView(Device* self, void* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* description, std::uint64_t destination) {
    if (destination == 0 || destination % 16 != 0) throw std::invalid_argument("Xbox shader view destination is null or misaligned");
    auto* output = reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination));
    if (resource) {
        auto* object = Resource::From(resource, self);
        const auto& metadata = object->GetMetadata();
        if (metadata.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
            CreateTextureShaderView(*object, description, destination);
            return;
        }
        if (!description) ANYBOX_NOT_IMPLEMENTED();
        if (description->ViewDimension != D3D12_SRV_DIMENSION_BUFFER) ANYBOX_NOT_IMPLEMENTED();
        if (description->Buffer.StructureByteStride != 0) throw std::invalid_argument("Xbox raw shader view cannot have a structure stride");
        if ((metadata.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) != 0) throw std::invalid_argument("Xbox resource does not allow shader resource views");
        std::array<std::uint32_t, 8> words;
        if (description->Buffer.Flags == D3D12_BUFFER_SRV_FLAG_NONE) words = EncodeTypedBuffer(Resource::GetGPUVirtualAddress(object), metadata.Width, *description);
        else if (description->Format == DXGI_FORMAT_R32_TYPELESS && description->Buffer.Flags == D3D12_BUFFER_SRV_FLAG_RAW) words = EncodeRawBuffer(Resource::GetGPUVirtualAddress(object), metadata.Width, description->Buffer.FirstElement, description->Buffer.NumElements, description->Shader4ComponentMapping);
        else ANYBOX_NOT_IMPLEMENTED();
        std::memcpy(output, words.data(), sizeof(words));
        return;
    }
    if (description && description->ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE) {
        std::array<std::uint64_t, 2> words{description->RaytracingAccelerationStructure.Location & 0xFFFFFFFFFFFFULL, 0x1400000000000ULL};
        std::memcpy(output, words.data(), sizeof(words));
    } else std::memset(output, 0, 32);
}
