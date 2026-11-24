#include "Resource.hpp"
#include "Texture.hpp"
#include "../object/PrivateData.hpp"

AnyBox::Graphics::Resource::Resource(BufferBacking backing, Device* device, std::uintptr_t address, const D3D12_RESOURCE_DESC1& description) : vtable(&GetResourceTable()), buffer(backing.memory->resource.get()), device(device), address(address), description(description) {
    auto owned = std::make_unique<BufferBacking>(std::move(backing));
    Device::AddRef(device);
    bufferBacking = owned.release();
}

AnyBox::Graphics::Resource::Resource(std::unique_ptr<TextureBacking> backing, Device* device, const D3D12_RESOURCE_DESC1& description) : vtable(&GetResourceTable()), buffer(nullptr), device(device), address(0), description(description), texture(backing.get()) {
    Device::AddRef(device);
    backing.release();
}

AnyBox::Graphics::Resource::~Resource() {
    privateData.Reset();
    delete bufferBacking;
    delete texture;
    Device::Release(device);
}

bool AnyBox::Graphics::Resource::Supports(const Guid& id) noexcept { return id == ResourceId || id == Resource1Id || id == Resource2Id || id == UnknownId || id == ObjectId || id == DeviceChildId || id == PageableId; }
