#include "Heap.hpp"
#include <bit>
#include <utility>

AnyBox::Graphics::DescriptorHeap::DescriptorHeap(DescriptorBacking backing, Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& description) : vtable(&GetDescriptorHeapTable()), type(description.Type), count(description.NumDescriptors), strideShift(std::bit_width(Device::GetDescriptorHandleIncrementSize(device, type)) - 1), address(reinterpret_cast<std::uintptr_t>(backing.address)), flags(description.Flags), allocation(backing.address), allocationSize(backing.size), attributes(backing.attributes), addressMirror(address), device(device), buffer(backing.buffer.get()) {
    auto retained = std::make_unique<DescriptorLease>();
    retained->memory = std::make_shared<DescriptorMemory>(device, std::move(backing));
    if (buffer && type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && (flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0) retained->mapping = device->GetDescriptorAddresses().Register(address, buffer->GetSize(), buffer->GetAddress(), retained->memory);
    Device::AddRef(device);
    lease = retained.release();
}

AnyBox::Graphics::DescriptorHeap::~DescriptorHeap() {
    privateData.Reset();
    delete lease;
    Device::Release(device);
}

bool AnyBox::Graphics::DescriptorHeap::Supports(const Guid& id) noexcept { return id == DescriptorHeapId || id == UnknownId || id == ObjectId || id == DeviceChildId || id == PageableId; }
