#include "Heap.hpp"
#include <string>
#include <utility>

namespace {
template<std::size_t TSlot>
[[noreturn]] void Unsupported(void*, ...) { throw std::runtime_error(std::string(ANYBOX_LIBRARY_NAME) + "!QueryHeap method " + std::to_string(TSlot) + " is not implemented"); }

template<std::size_t... TSlots>
AnyBox::Graphics::ObjectTable MakeTable(std::index_sequence<TSlots...>) {
    using namespace AnyBox::Graphics;
    ObjectTable result{MethodAddress(&Unsupported<TSlots>)...};
    result[0] = MethodAddress(&QueryHeap::QueryInterface);
    result[1] = MethodAddress(&QueryHeap::AddRef);
    result[2] = MethodAddress(&QueryHeap::Release);
    result[3] = MethodAddress(&QueryHeap::GetPrivateData);
    result[4] = MethodAddress(&QueryHeap::SetPrivateData);
    result[5] = MethodAddress(&QueryHeap::SetPrivateDataInterface);
    result[6] = MethodAddress(&QueryHeap::SetName);
    result[7] = MethodAddress(&QueryHeap::GetDevice);
    return result;
}
}

const AnyBox::Graphics::ObjectTable& AnyBox::Graphics::GetQueryHeapTable() {
    static const auto table = MakeTable(std::make_index_sequence<std::tuple_size_v<ObjectTable>>{});
    return table;
}
