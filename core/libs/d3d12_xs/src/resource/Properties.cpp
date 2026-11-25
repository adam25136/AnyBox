#include "Resource.hpp"

AnyBox::Graphics::Resource* AnyBox::Graphics::Resource::From(void* value, Device* owner) {
    if (!value || reinterpret_cast<std::uintptr_t>(value) % alignof(Resource) != 0) throw std::invalid_argument("Xbox resource pointer is null or misaligned");
    const ObjectTable* table = nullptr;
    std::memcpy(&table, value, sizeof(table));
    if (table != &GetResourceTable()) throw std::invalid_argument("Object is not an AnyBox graphics resource");
    auto* resource = static_cast<Resource*>(value);
    if (resource->device != owner) throw std::invalid_argument("Xbox resource belongs to another graphics device");
    return resource;
}

D3D12_RESOURCE_DESC AnyBox::Graphics::Resource::GetDescription(Resource* self) {
    D3D12_RESOURCE_DESC value{};
    static_assert(offsetof(D3D12_RESOURCE_DESC, Flags) == offsetof(D3D12_RESOURCE_DESC1, Flags));
    std::memcpy(&value, &self->description, offsetof(D3D12_RESOURCE_DESC, Flags) + sizeof(value.Flags));
    return value;
}

D3D12_RESOURCE_DESC1 AnyBox::Graphics::Resource::GetDescription1(Resource* self) { return self->description; }
std::uint64_t AnyBox::Graphics::Resource::GetGPUVirtualAddress(Resource* self) noexcept { return self->address; }

HRESULT AnyBox::Graphics::Resource::Map(Resource* self, std::uint32_t subresource, const D3D12_RANGE* range, void** data) {
    if (data) *data = nullptr;
    if (!self->buffer) throw std::runtime_error("Default heap textures cannot be mapped");
    if (self->bufferBacking->memory->properties && self->bufferBacking->memory->properties->Type == D3D12_HEAP_TYPE_DEFAULT) throw std::runtime_error("Default heap committed buffers cannot be mapped");
    if (subresource != 0 || (range && (range->Begin > range->End || range->End > self->description.Width))) throw std::invalid_argument("Placed buffer map range is invalid");
    if (data) *data = self->buffer->Map().data();
    return S_OK;
}

void AnyBox::Graphics::Resource::Unmap(Resource* self, std::uint32_t subresource, const D3D12_RANGE* range) {
    if (!self->buffer) throw std::runtime_error("Default heap textures cannot be unmapped");
    if (self->bufferBacking->memory->properties && self->bufferBacking->memory->properties->Type == D3D12_HEAP_TYPE_DEFAULT) throw std::runtime_error("Default heap committed buffers cannot be unmapped");
    if (subresource != 0 || (range && (range->Begin > range->End || range->End > self->description.Width))) throw std::invalid_argument("Placed buffer unmap range is invalid");
}
