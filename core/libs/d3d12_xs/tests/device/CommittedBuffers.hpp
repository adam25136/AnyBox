#pragma once

#include "Tests.hpp"
#include "../../src/resource/Resource.hpp"
#include <memory>

namespace AnyBox::Graphics::Tests {
struct CommittedBufferDeleter {
    void operator()(Resource* value) const { Method<decltype(&Resource::Release)>(value, 2)(value); }
};
using OwnedCommittedBuffer = std::unique_ptr<Resource, CommittedBufferDeleter>;

inline D3D12_RESOURCE_DESC CommittedDescription(std::uint64_t size) {
    D3D12_RESOURCE_DESC result{};
    result.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    result.Width = size;
    result.Height = 1;
    result.DepthOrArraySize = 1;
    result.MipLevels = 1;
    result.SampleDesc.Count = 1;
    result.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    return result;
}

inline D3D12_HEAP_PROPERTIES CommittedProperties(D3D12_HEAP_TYPE type) {
    D3D12_HEAP_PROPERTIES result{};
    result.Type = type;
    result.CreationNodeMask = 1;
    result.VisibleNodeMask = 1;
    return result;
}

inline OwnedCommittedBuffer MakeCommittedBuffer(Device* device, D3D12_HEAP_TYPE type, std::uint64_t size, bool enhanced = false) {
    auto properties = CommittedProperties(type);
    auto description = CommittedDescription(size);
    void* output = nullptr;
    if (enhanced) {
        D3D12_RESOURCE_DESC1 extended{};
        std::memcpy(&extended, &description, offsetof(D3D12_RESOURCE_DESC, Flags) + sizeof(description.Flags));
        auto create = Method<decltype(&Device::CreateCommittedResource3)>(device, 127);
        Require(create(device, &properties, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, &extended, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr, nullptr, 0, nullptr, ResourceId, &output) == S_OK && output, "Enhanced committed buffer creation failed");
    } else {
        auto state = type == D3D12_HEAP_TYPE_READBACK ? D3D12_RESOURCE_STATE_COPY_DEST : type == D3D12_HEAP_TYPE_UPLOAD ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;
        auto create = Method<decltype(&Device::CreateCommittedResource)>(device, 27);
        Require(create(device, &properties, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, &description, state, nullptr, ResourceId, &output) == S_OK && output, "Legacy committed buffer creation failed");
    }
    return OwnedCommittedBuffer(static_cast<Resource*>(output));
}
}
