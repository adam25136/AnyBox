#pragma once

#include "Device.hpp"

namespace AnyBox::Graphics {
class DeviceReference {
public:
    explicit DeviceReference(Device* device) : device(device) { Device::AddRef(device); }
    ~DeviceReference() { Device::Release(device); }
    DeviceReference(const DeviceReference&) = delete;
    DeviceReference& operator=(const DeviceReference&) = delete;

private:
    Device* device;
};
}
