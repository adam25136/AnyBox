#pragma once

#include "../device/Device.hpp"
#include "../object/Lifetime.hpp"

namespace AnyBox::Graphics {
inline constexpr Guid QueryHeapId{0x0D9658AE, 0xED45, 0x469E, {0xA6, 0x1D, 0x97, 0x0E, 0xC5, 0x83, 0xCA, 0xB4}};
const ObjectTable& GetQueryHeapTable();

class QueryHeap : public ObjectLifetime<QueryHeap> {
public:
    QueryHeap(ID3D12QueryHeap* host, Device* device, const D3D12_QUERY_HEAP_DESC& description);
    ~QueryHeap();
    static constexpr std::size_t GetVtableOffset() { return offsetof(QueryHeap, vtable); }
    ID3D12QueryHeap* GetHost() const noexcept { return host; }
    const D3D12_QUERY_HEAP_DESC& GetDescription() const noexcept { return description; }
    static QueryHeap* From(void* object);
    static bool Supports(const Guid& interfaceId) noexcept;

private:
    friend class ObjectLifetime<QueryHeap>;
    const ObjectTable* vtable;
    std::atomic<std::uint32_t>& getReferences() noexcept { return references; }
    std::atomic<std::uint32_t> references{1};
    ID3D12QueryHeap* host;
    Device* device;
    D3D12_QUERY_HEAP_DESC description;
};

static_assert(std::is_standard_layout_v<QueryHeap>);
static_assert(QueryHeap::GetVtableOffset() == 0);
}
