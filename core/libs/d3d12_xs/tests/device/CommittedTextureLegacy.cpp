#include "CommittedBuffers.hpp"
#include <anybox/vulkan/Image.hpp>

void AnyBox::Graphics::Tests::CheckCommittedTextureLegacy(Device* device) {
    auto create = Method<decltype(&Device::CreateCommittedResource)>(device, 27);
    auto properties = CommittedProperties(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC description{};
    description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    description.Width = 16;
    description.Height = 32;
    description.DepthOrArraySize = 1;
    description.MipLevels = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    void* output = nullptr;
    Require(create(device, &properties, D3D12_HEAP_FLAG_NONE, &description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, ResourceId, &output) == S_OK && output, "Legacy committed texture creation failed");
    std::unique_ptr<Resource, CommittedBufferDeleter> resource(static_cast<Resource*>(output));
    const auto& metadata = resource->GetMetadata();
    Require(metadata.Width == 16 && metadata.Height == 32 && metadata.Format == description.Format, "Legacy committed texture description changed");
    Require(resource->GetGpuImage().GetDescription().extent.height == 32, "Legacy committed texture image dimensions changed");
    void* mapped = resource.get();
    Reject<std::runtime_error>([&] { Method<decltype(&Resource::Map)>(resource.get(), 8)(resource.get(), 0, nullptr, &mapped); });
    Require(!mapped, "Rejected legacy texture mapping retained a stale pointer");
    Reject<std::runtime_error>([&] { create(device, &properties, D3D12_HEAP_FLAG_NONE, &description, D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr, ResourceId, &output); });
}
