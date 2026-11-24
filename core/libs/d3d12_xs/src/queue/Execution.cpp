#include "Queue.hpp"
#include "../list/List.hpp"
#include <array>
#include <algorithm>
#include <functional>
#include <span>
#include <stdexcept>
#include <vector>

void AnyBox::Graphics::Queue::ExecuteCommandLists(Queue* self, std::uint32_t count, void* const* lists) {
    if (count != 0 && !lists) throw std::invalid_argument("Command list array is null");
    std::array<ID3D12CommandList*, 16> local;
    std::array<ID3D12CommandList*, 16> localSorted;
    std::span<ID3D12CommandList*> native(local);
    std::span<ID3D12CommandList*> sorted(localSorted);
    if (count > local.size()) {
        thread_local std::vector<ID3D12CommandList*> expanded;
        expanded.resize(static_cast<std::size_t>(count) * 2);
        native = std::span(expanded).first(count);
        sorted = std::span(expanded).subspan(count);
    }
    auto type = self->description.Type;
    bool hasGpu = false;
    if (static_cast<std::uint32_t>(type) == 0x10) type = D3D12_COMMAND_LIST_TYPE_COPY;
    for (std::uint32_t index = 0; index < count; ++index) {
        auto* list = CommandList::From(lists[index]);
        if (list->GetParent() != self->device) throw std::invalid_argument("Command list and queue devices differ");
        if (!list->IsClosed()) throw std::logic_error("Recording command list cannot be executed");
        if (list->GetNativeType() != type) throw std::invalid_argument("Command list and queue types differ");
        native[index] = list->GetHost();
        hasGpu |= list->GetGpuRecording() != nullptr;
    }
    if (count > 1) {
        auto values = sorted.first(count);
        std::ranges::copy(native.first(count), values.begin());
        std::ranges::sort(values, std::less<ID3D12CommandList*>{});
        if (std::ranges::adjacent_find(values) != values.end()) throw std::runtime_error("Repeated command lists within one batch are not implemented");
    }
    auto lock = self->lockCommands();
    if (hasGpu) self->executeGpu(std::span(lists, count), native.first(count));
    else self->host->ExecuteCommandLists(count, count == 0 ? nullptr : native.data());
}
