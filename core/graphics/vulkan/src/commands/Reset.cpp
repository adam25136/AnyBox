#include "State.hpp"
#include <limits>

using namespace AnyBox::Vulkan;

CommandReset::CommandReset(std::shared_ptr<CommandArenaState> state) : state(std::move(state)), lock(this->state->mutex) {
    this->state->Check(this->state->generation);
    if (this->state->recording) throw std::logic_error("Command allocator cannot be reset while recording");
    for (const auto& pending : this->state->pending) if (!pending.IsComplete()) throw std::logic_error("Command allocator cannot be reset while GPU work is pending");
    bool submitted = false;
    for (const auto& pending : this->state->pending) submitted |= pending.GetValue() != 0;
    if (submitted) for (const auto& check : this->state->checks) check->CheckCompletion();
    if (this->state->generation == std::numeric_limits<std::uint64_t>::max()) throw std::overflow_error("Command allocator generations are exhausted");
}

void CommandReset::Commit() {
    if (committed) throw std::logic_error("Command allocator reset was already committed");
    state->failed = true;
    Check(state->context.GetFunctions().resetCommandPool(state->context.GetDevice(), state->pool, 0), "Reset Vulkan command arena");
    ++state->generation;
    state->cursor = 0;
    state->pending.fill({});
    state->resources.clear();
    state->checks.clear();
    state->failed = false;
    committed = true;
}
