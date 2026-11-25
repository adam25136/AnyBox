#include "Device.hpp"
#include "../pagepool/Registry.hpp"

HRESULT AnyBox::Graphics::Device::RegisterPagePoolX(Device* self, std::uint64_t address, std::uint32_t count, std::uint64_t* handle) {
    if (!handle) throw std::invalid_argument("Xbox page pool handle output is null");
    *handle = 0;
    if (!self) throw std::invalid_argument("Xbox page pool device is null");
    *handle = self->pagePools->Register(address, count);
    return S_OK;
}

HRESULT AnyBox::Graphics::Device::UnregisterPagePoolX(Device* self, std::uint64_t handle) {
    if (!self) throw std::invalid_argument("Xbox page pool device is null");
    self->pagePools->Unregister(handle);
    return S_OK;
}
