#include "Allocator.hpp"
#include <stdexcept>

AnyBox::Graphics::Allocator::Allocator(ID3D12CommandAllocator* host, Device* device, D3D12_COMMAND_LIST_TYPE type) : vtable(&GetAllocatorTable()), host(host), device(device), type(type) { Device::AddRef(device); }

AnyBox::Graphics::Allocator::~Allocator() {
    delete gpu;
    host->Release();
    Device::Release(device);
}

AnyBox::Graphics::Allocator* AnyBox::Graphics::Allocator::From(void* object) {
    if (!object) throw std::invalid_argument("Allocator is null");
    const ObjectTable* table;
    std::memcpy(&table, object, sizeof(table));
    if (table != &GetAllocatorTable()) throw std::invalid_argument("Foreign fence interface is unsupported");
    return static_cast<Allocator*>(object);
}

bool AnyBox::Graphics::Allocator::Supports(const Guid& interfaceId) noexcept { return interfaceId == AllocatorId || interfaceId == UnknownId || interfaceId == ObjectId || interfaceId == DeviceChildId || interfaceId == PageableId; }

HRESULT AnyBox::Graphics::Allocator::Reset(Allocator* self) {
    std::scoped_lock lock(self->gpuMutex);
    if (self->gpu) {
        auto reset = self->gpu->LockReset();
        auto result = self->host->Reset();
        CheckGraphicsResult(result, "ID3D12CommandAllocator::Reset");
        reset.Commit();
        return result;
    }
    auto result = self->host->Reset();
    CheckGraphicsResult(result, "ID3D12CommandAllocator::Reset");
    return result;
}
