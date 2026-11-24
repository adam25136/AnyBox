#include "List.hpp"
#include <stdexcept>
#include <string>
#include <utility>

namespace {
template<std::size_t TSlot>
[[noreturn]] void Unsupported(void*, ...) { throw std::runtime_error(std::string(ANYBOX_LIBRARY_NAME) + "!CommandList method " + std::to_string(TSlot) + " is not implemented"); }

template<std::size_t... TSlots>
AnyBox::Graphics::ListTable MakeTable(std::index_sequence<TSlots...>) {
    using namespace AnyBox::Graphics;
    ListTable result{MethodAddress(&Unsupported<TSlots>)...};
    result[0] = MethodAddress(&CommandList::QueryInterface);
    result[1] = MethodAddress(&CommandList::AddRef);
    result[2] = MethodAddress(&CommandList::Release);
    result[3] = MethodAddress(&CommandList::GetPrivateData);
    result[4] = MethodAddress(&CommandList::SetPrivateData);
    result[5] = MethodAddress(&CommandList::SetPrivateDataInterface);
    result[6] = MethodAddress(&CommandList::SetName);
    result[7] = MethodAddress(&CommandList::GetDevice);
    result[8] = MethodAddress(&CommandList::GetType);
    result[9] = MethodAddress(&CommandList::Close);
    result[10] = MethodAddress(&CommandList::Reset);
    result[14] = MethodAddress(&CommandList::Dispatch);
    result[25] = MethodAddress(&CommandList::SetPipelineState);
    result[33] = MethodAddress(&CommandList::SetComputeRootSignature);
    result[35] = MethodAddress(&CommandList::SetComputeRootDescriptorTable);
    result[37] = MethodAddress(&CommandList::SetComputeRoot32BitConstant);
    result[39] = MethodAddress(&CommandList::SetComputeRoot32BitConstants);
    result[41] = MethodAddress(&CommandList::SetComputeRootConstantBufferView);
    result[43] = MethodAddress(&CommandList::SetComputeRootShaderResourceView);
    result[45] = MethodAddress(&CommandList::SetComputeRootUnorderedAccessView);
    result[131] = MethodAddress(&CommandList::SyncTopOfPipeX);
    result[132] = MethodAddress(&CommandList::SyncBottomOfPipeX);
    return result;
}
}

const AnyBox::Graphics::ListTable& AnyBox::Graphics::GetListTable() {
    static const auto table = MakeTable(std::make_index_sequence<256>{});
    return table;
}
