#include "Tests.hpp"
#include "../../src/query/Heap.hpp"
#include <string>

void AnyBox::Graphics::Tests::CheckQueryHeaps(Device* device) {
    auto create = Method<decltype(&Device::CreateQueryHeap)>(device, 39);
    for (auto type : {D3D12_QUERY_HEAP_TYPE_OCCLUSION, D3D12_QUERY_HEAP_TYPE_TIMESTAMP, D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS}) {
        D3D12_QUERY_HEAP_DESC description{type, 8, 1};
        auto instantiate = [&](void** destination) {
            try { return create(device, &description, QueryHeapId, destination); }
            catch (const std::exception& error) { throw std::runtime_error("Query heap type=" + std::to_string(type) + " validation=" + std::to_string(destination == nullptr) + ": " + error.what()); }
        };
        void* output = nullptr;
        Require(instantiate(&output) == S_OK && output, "Query heap creation failed");
        auto* heap = static_cast<QueryHeap*>(output);
        auto release = Method<decltype(&QueryHeap::Release)>(heap, 2);
        const auto& stored = heap->GetDescription();
        Require(stored.Type == type && stored.Count == 8 && stored.NodeMask == 1, "Query heap description changed across the Xbox ABI");
        auto query = Method<decltype(&QueryHeap::QueryInterface)>(heap, 0);
        for (const auto& id : {QueryHeapId, UnknownId, ObjectId, DeviceChildId, PageableId}) {
            void* identity = nullptr;
            Require(query(heap, id, &identity) == S_OK && identity == heap, "Query heap COM identity changed");
            Require(release(static_cast<QueryHeap*>(identity)) == 1, "Query heap interface did not own its reference");
        }
        void* parent = nullptr;
        Require(Method<decltype(&QueryHeap::GetDevice)>(heap, 7)(heap, XboxDeviceId, &parent) == S_OK && parent == device, "Query heap parent identity changed");
        Method<decltype(&Device::Release)>(device, 2)(static_cast<Device*>(parent));
        void* rejected = heap;
        Reject<std::invalid_argument>([&] { query(heap, Guid{}, &rejected); });
        Require(!rejected, "Rejected query heap interface retained a stale pointer");
        Reject<std::invalid_argument>([&] { query(heap, QueryHeapId, nullptr); });
        Require(Method<decltype(&QueryHeap::SetName)>(heap, 6)(heap, L"AnyBox query heap") == S_OK, "Query heap name was not forwarded");
        Guid key{0x12345678, 1, 2, {3, 4, 5, 6, 7, 8, 9, 10}};
        std::uint32_t value = 0xABCDEF01;
        Require(Method<decltype(&QueryHeap::SetPrivateData)>(heap, 4)(heap, key, sizeof(value), &value) == S_OK, "Query heap private data was not stored");
        std::uint32_t copied = 0;
        std::uint32_t size = sizeof(copied);
        Require(Method<decltype(&QueryHeap::GetPrivateData)>(heap, 3)(heap, key, &size, &copied) == S_OK && copied == value, "Query heap private data changed");
        Require(release(heap) == 0, "Query heap final reference was not released");
        GUID nativeId;
        std::memcpy(&nativeId, &QueryHeapId, sizeof(nativeId));
        auto validation = device->GetHost()->CreateQueryHeap(&description, nativeId, nullptr);
        if (SUCCEEDED(validation)) Require(instantiate(nullptr) == validation, "Query heap validation mode did not preserve the host result");
        else Reject<std::runtime_error>([&] { instantiate(nullptr); });
    }
    D3D12_QUERY_HEAP_DESC description{D3D12_QUERY_HEAP_TYPE_OCCLUSION, 8, 1};
    void* output = device;
    Reject<std::invalid_argument>([&] { create(device, nullptr, QueryHeapId, &output); });
    Require(!output, "Rejected query heap creation retained a stale pointer");
    description.Count = 0;
    Reject<std::invalid_argument>([&] { create(device, &description, QueryHeapId, &output); });
    description.Count = 8;
    description.NodeMask = 2;
    Reject<std::invalid_argument>([&] { create(device, &description, QueryHeapId, &output); });
    description.NodeMask = 1;
    Reject<std::invalid_argument>([&] { create(device, &description, Guid{}, &output); });
}
