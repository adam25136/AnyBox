#include "Device.hpp"
#include "../query/Heap.hpp"
#include <memory>

namespace {
struct QueryHeapDeleter {
    void operator()(ID3D12QueryHeap* value) const noexcept { value->Release(); }
};
}

HRESULT AnyBox::Graphics::Device::CreateQueryHeap(Device* self, const D3D12_QUERY_HEAP_DESC* description, const Guid& interfaceId, void** heap) {
    if (heap) *heap = nullptr;
    if (!description) throw std::invalid_argument("Query heap description is null");
    if (description->Count == 0 || description->NodeMask > 1) throw std::invalid_argument("Query heap count or node mask is invalid");
    if (!QueryHeap::Supports(interfaceId)) throw std::invalid_argument("Unsupported query heap creation interface");
    void* host = nullptr;
    auto result = self->host->CreateQueryHeap(description, NativeGuid(QueryHeapId), heap ? &host : nullptr);
    CheckGraphicsResult(result, "ID3D12Device::CreateQueryHeap");
    if (!heap) return result;
    if (!host) throw std::runtime_error("D3D12 query heap creation returned no interface");
    std::unique_ptr<ID3D12QueryHeap, QueryHeapDeleter> ownedHost(static_cast<ID3D12QueryHeap*>(host));
    auto wrapper = std::make_unique<QueryHeap>(ownedHost.get(), self, *description);
    ownedHost.release();
    *heap = wrapper.release();
    return result;
}
