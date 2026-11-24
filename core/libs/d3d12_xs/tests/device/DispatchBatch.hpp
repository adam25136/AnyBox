#pragma once

#include "Tests.hpp"

namespace AnyBox::Graphics::Tests {
class ComputeFixture;
class GpuQueueFixture;
class DispatchBuffer;
void CheckDispatchBatch(Device* device, ComputeFixture& fixture, void* pipeline, GpuQueueFixture& queue, DispatchBuffer& buffer);
}
