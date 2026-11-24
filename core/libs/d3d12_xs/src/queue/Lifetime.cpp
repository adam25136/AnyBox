#include "Queue.hpp"
#include <anybox/vulkan/QueueBridge.hpp>
#include <stdexcept>

AnyBox::Graphics::Queue::Queue(ID3D12CommandQueue* host, Device* device, const D3D12_COMMAND_QUEUE_DESC& description) : vtable(&GetQueueTable()), host(host), device(device), description(description) { Device::AddRef(device); }

AnyBox::Graphics::Queue::~Queue() {
    delete gpu;
    if (gpuQueueIndex != UINT32_MAX) device->ReleaseGpuQueue(gpuQueueIndex);
    host->Release();
    if (xboxSelection) device->ReleaseQueueSelection(xboxSelection);
    Device::Release(device);
}

bool AnyBox::Graphics::Queue::Supports(const Guid& interfaceId) noexcept { return interfaceId == QueueId || interfaceId == UnknownId || interfaceId == ObjectId || interfaceId == DeviceChildId || interfaceId == PageableId; }

