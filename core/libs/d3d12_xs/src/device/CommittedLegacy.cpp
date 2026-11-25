#include "Device.hpp"
#include "../resource/Resource.hpp"
#include <utility>

namespace {
D3D12_BARRIER_LAYOUT TextureLayout(D3D12_RESOURCE_STATES state) {
    switch (state) {
    case D3D12_RESOURCE_STATE_COMMON: return D3D12_BARRIER_LAYOUT_COMMON;
    case D3D12_RESOURCE_STATE_COPY_SOURCE: return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
    case D3D12_RESOURCE_STATE_COPY_DEST: return D3D12_BARRIER_LAYOUT_COPY_DEST;
    case D3D12_RESOURCE_STATE_RENDER_TARGET: return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    case D3D12_RESOURCE_STATE_UNORDERED_ACCESS: return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
    case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
    case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
    case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE: return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
    default: throw std::runtime_error("Committed texture initial resource state is not implemented");
    }
}
}

HRESULT AnyBox::Graphics::Device::CreateCommittedResource(Device* self, const D3D12_HEAP_PROPERTIES* properties, D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC* description, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue, const Guid& interfaceId, void** resource) {
    if (!resource) throw std::invalid_argument("Committed resource output is null");
    *resource = nullptr;
    if (!self || !properties || !description) throw std::invalid_argument("Committed resource creation arguments are null");
    if (!Resource::Supports(interfaceId)) throw std::invalid_argument("Committed resource interface is unsupported");
    D3D12_RESOURCE_DESC1 extended{};
    std::memcpy(&extended, description, offsetof(D3D12_RESOURCE_DESC, Flags) + sizeof(description->Flags));
    std::unique_ptr<Resource> wrapper;
    if (description->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
        if (clearValue) throw std::invalid_argument("Committed buffers cannot have an optimized clear value");
        auto backing = CreateCommittedBufferBacking(self, *properties, flags, extended, state);
        auto address = reinterpret_cast<std::uintptr_t>(backing.memory->allocation->GetAddress());
        wrapper = std::make_unique<Resource>(std::move(backing), self, address, extended);
    } else {
        auto backing = CreateTextureBacking(self, *properties, flags, extended, TextureLayout(state), clearValue);
        wrapper = std::make_unique<Resource>(std::move(backing), self, extended);
    }
    *resource = wrapper.release();
    return S_OK;
}
