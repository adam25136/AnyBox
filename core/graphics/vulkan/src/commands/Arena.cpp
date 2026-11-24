#include "State.hpp"

using namespace AnyBox::Vulkan;

CommandArena::CommandArena(Context& context) : state(std::make_shared<CommandArenaState>(context)) {}

CommandRecord CommandArena::Begin() {
    std::scoped_lock lock(state->mutex);
    state->Check(state->generation);
    if (state->recording) throw std::logic_error("Command allocator already has a recording command list");
    auto& functions = state->context.GetFunctions();
    if (state->cursor == state->buffers.size()) {
        state->buffers.push_back(VK_NULL_HANDLE);
        VkCommandBufferAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocation.commandPool = state->pool;
        allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocation.commandBufferCount = 1;
        auto result = functions.allocateCommandBuffers(state->context.GetDevice(), &allocation, &state->buffers.back());
        if (result != VK_SUCCESS) {
            state->buffers.pop_back();
            Check(result, "Allocate Vulkan arena command buffer");
        }
    }
    auto commands = state->buffers[state->cursor];
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    state->failed = true;
    Check(functions.beginCommandBuffer(commands, &begin), "Begin Vulkan arena command buffer");
    state->failed = false;
    ++state->cursor;
    state->recording = true;
    return CommandRecord(state, commands, state->generation);
}

CommandReset CommandArena::LockReset() { return CommandReset(state); }
CommandSubmission CommandArena::LockSubmission(const CommandRecord& record, const QueueBridge& queue) { return CommandSubmission(state, record, queue); }
void CommandArena::Reset() { auto reset = LockReset(); reset.Commit(); }
