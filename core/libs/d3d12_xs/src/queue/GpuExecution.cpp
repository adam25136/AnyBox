#include "Queue.hpp"
#include "../list/List.hpp"
#include "../list/Gpu.hpp"
#include <algorithm>
#include <optional>

namespace {
struct Entry {
    AnyBox::Graphics::GpuRecording* recording;
    std::size_t index;
};

class Locks {
public:
    explicit Locks(std::span<std::optional<AnyBox::Vulkan::CommandSubmission>> slots) : slots(slots) {}
    ~Locks() { for (auto index = slots.size(); index != 0; --index) slots[index - 1].reset(); }

private:
    std::span<std::optional<AnyBox::Vulkan::CommandSubmission>> slots;
};
}

void AnyBox::Graphics::Queue::executeGpu(std::span<void* const> lists, std::span<ID3D12CommandList*> native) {
    std::array<Entry, 16> localEntries;
    std::array<VkCommandBuffer, 16> localCommands{};
    std::array<std::optional<Vulkan::CommandSubmission>, 16> localLocks;
    std::span<Entry> entries(localEntries);
    std::span<VkCommandBuffer> commands(localCommands);
    std::span<std::optional<Vulkan::CommandSubmission>> locks(localLocks);
    if (lists.size() > localEntries.size()) {
        thread_local std::vector<Entry> expandedEntries;
        thread_local std::vector<VkCommandBuffer> expandedCommands;
        thread_local std::vector<std::optional<Vulkan::CommandSubmission>> expandedLocks;
        expandedEntries.resize(lists.size());
        expandedCommands.assign(lists.size(), VK_NULL_HANDLE);
        expandedLocks.resize(lists.size());
        entries = expandedEntries;
        commands = expandedCommands;
        locks = expandedLocks;
    }
    Locks release(locks);
    std::size_t count = 0;
    for (std::size_t index = 0; index < lists.size(); ++index) {
        auto* recording = CommandList::From(lists[index])->GetGpuRecording();
        if (!recording) continue;
        recording->Check();
        entries[count++] = {recording, index};
    }
    auto selected = entries.first(count);
    std::sort(selected.begin(), selected.end(), [](const auto& left, const auto& right) { return std::less<Vulkan::CommandArena*>{}(&left.recording->GetArena(), &right.recording->GetArena()); });
    auto& bridge = getGpuBridge();
    Vulkan::CommandArena* previous = nullptr;
    std::size_t used = 0;
    for (const auto& entry : selected) {
        auto& arena = entry.recording->GetArena();
        const auto& record = entry.recording->GetRecord();
        if (previous != &arena) {
            locks[used++].emplace(arena.LockSubmission(record, bridge));
            previous = &arena;
        }
        commands[entry.index] = locks[used - 1]->Validate(record);
    }
    for (std::size_t begin = 0; begin < lists.size();) {
        auto end = begin + 1;
        auto gpuRange = commands[begin] != VK_NULL_HANDLE;
        while (end < lists.size() && (commands[end] != VK_NULL_HANDLE) == gpuRange) ++end;
        host->ExecuteCommandLists(static_cast<UINT>(end - begin), native.data() + begin);
        if (gpuRange) bridge.Submit(commands.subspan(begin, end - begin));
        begin = end;
    }
}
