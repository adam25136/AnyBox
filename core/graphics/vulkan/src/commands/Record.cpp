#include "State.hpp"
#include <utility>

using namespace AnyBox::Vulkan;

CommandRecord::CommandRecord(std::shared_ptr<CommandArenaState> state, VkCommandBuffer commands, std::uint64_t generation) : state(std::move(state)), commands(commands), generation(generation) {}
CommandRecord::CommandRecord(CommandRecord&& other) noexcept : state(std::move(other.state)), commands(other.commands), generation(other.generation), recording(std::exchange(other.recording, false)) {}

CommandRecord::~CommandRecord() {
    if (state && recording) { std::scoped_lock lock(state->mutex); state->recording = false; }
}

VkCommandBuffer CommandRecord::GetCommands() const {
    if (!state) throw std::logic_error("Vulkan command record is empty");
    std::scoped_lock lock(state->mutex);
    state->Check(generation);
    if (!recording) throw std::logic_error("Vulkan command record is closed");
    return commands;
}

VkCommandBuffer CommandRecord::GetExecutable() const {
    if (!state) throw std::logic_error("Vulkan command record is empty");
    std::scoped_lock lock(state->mutex);
    state->Check(generation);
    if (recording) throw std::logic_error("Vulkan command record is not closed");
    return commands;
}

void CommandRecord::Close() {
    if (!state) throw std::logic_error("Vulkan command record is empty");
    std::scoped_lock lock(state->mutex);
    state->Check(generation);
    if (!recording) throw std::logic_error("Vulkan command record is already closed");
    state->failed = true;
    Check(state->context.GetFunctions().endCommandBuffer(commands), "Close Vulkan arena command buffer");
    state->failed = false;
    state->recording = recording = false;
}

void CommandRecord::Retain(std::shared_ptr<void> resource) {
    if (!state) throw std::logic_error("Vulkan command record is empty");
    std::scoped_lock lock(state->mutex);
    state->Check(generation);
    if (!recording || !resource) throw std::invalid_argument("Vulkan command resource requires an open record and a nonnull owner");
    state->resources.push_back(std::move(resource));
}

void CommandRecord::Verify(std::shared_ptr<ICompletionCheck> check) {
    if (!state) throw std::logic_error("Vulkan command record is empty");
    std::scoped_lock lock(state->mutex);
    state->Check(generation);
    if (!recording || !check) throw std::invalid_argument("Vulkan completion check requires an open record and a nonnull owner");
    state->checks.push_back(std::move(check));
}
