#include "Resource.hpp"
#include "Texture.hpp"
#include <anybox/Unimplemented.hpp>

HRESULT AnyBox::Graphics::Resource::GetHeapProperties(Resource* self, D3D12_HEAP_PROPERTIES* properties, D3D12_HEAP_FLAGS* flags) {
    if (!properties && !flags) throw std::invalid_argument("Resource heap property outputs are null");
    if (self->bufferBacking && self->bufferBacking->memory->properties) {
        if (properties) *properties = *self->bufferBacking->memory->properties;
        if (flags) *flags = self->bufferBacking->memory->flags;
        return S_OK;
    }
    if (!self->texture) ANYBOX_NOT_IMPLEMENTED();
    if (properties) *properties = self->texture->properties;
    if (flags) *flags = self->texture->flags;
    return S_OK;
}
