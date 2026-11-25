#pragma once

#include "../device/Device.hpp"
#include "Com.hpp"
#include <dxgi1_4.h>
#include <memory>

namespace AnyBox::Graphics {
using NativeAdapter = ComOwner<IDXGIAdapter1>;
ComOwner<IDXGIFactory4> CreateHostDxgiFactory();
NativeAdapter GetHostAdapter(Device* device);
}
