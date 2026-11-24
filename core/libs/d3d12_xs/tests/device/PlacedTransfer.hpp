#pragma once

#include "../../src/resource/Resource.hpp"

namespace AnyBox::Graphics::Tests {
void CheckPlacedBufferTransfer(Device* device, Resource* resource, std::span<std::byte> memory, bool repeated);
void CheckPlacedBufferLifetime(Device* device, Resource* resource, std::span<std::byte> memory);
}
