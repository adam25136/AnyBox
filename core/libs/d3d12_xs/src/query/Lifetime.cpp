#include "Heap.hpp"

AnyBox::Graphics::QueryHeap::QueryHeap(ID3D12QueryHeap* host, Device* device, const D3D12_QUERY_HEAP_DESC& description) : vtable(&GetQueryHeapTable()), host(host), device(device), description(description) {
    if (!host || !device) throw std::invalid_argument("Query heap host or parent device is null");
    Device::AddRef(device);
}

AnyBox::Graphics::QueryHeap::~QueryHeap() {
    host->Release();
    Device::Release(device);
}

AnyBox::Graphics::QueryHeap* AnyBox::Graphics::QueryHeap::From(void* object) {
    if (!object) throw std::invalid_argument("Query heap is null");
    const ObjectTable* table;
    std::memcpy(&table, object, sizeof(table));
    if (table != &GetQueryHeapTable()) throw std::invalid_argument("Foreign query heap interface is unsupported");
    return static_cast<QueryHeap*>(object);
}

bool AnyBox::Graphics::QueryHeap::Supports(const Guid& interfaceId) noexcept { return interfaceId == QueryHeapId || interfaceId == UnknownId || interfaceId == ObjectId || interfaceId == DeviceChildId || interfaceId == PageableId; }
