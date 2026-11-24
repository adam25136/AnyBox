#include "State.hpp"

using namespace AnyBox::Vulkan;

CommandSubmission::CommandSubmission(std::shared_ptr<CommandArenaState> state, const CommandRecord& record, const QueueBridge& queue) : state(std::move(state)), lock(this->state->mutex), queue(queue), slot(this->state->pending.size()), commands(record.commands) {
    if (record.state != this->state || &queue.GetContext() != &this->state->context) throw std::invalid_argument("Command submission belongs to another arena or device");
    this->state->Check(record.generation);
    if (record.recording) throw std::logic_error("Recording command list cannot be submitted");
    queue.Check();
    auto source = queue.GetCompletion().GetSource();
    for (std::size_t index = 0; index < this->state->pending.size(); ++index) {
        if (this->state->pending[index].GetSource() == source) { slot = index; return; }
        if (this->state->pending[index].GetSource() == nullptr) slot = index;
    }
    if (slot != this->state->pending.size()) return;
    for (std::size_t index = 0; index < this->state->pending.size(); ++index) {
        if (this->state->pending[index].IsComplete()) { slot = index; return; }
    }
    throw std::length_error("Command allocator pending queue capacity is exhausted");
}

CommandSubmission::~CommandSubmission() {
    auto completion = queue.GetCompletion();
    auto& pending = state->pending[slot];
    if (pending.GetSource() != completion.GetSource() || completion.GetValue() > pending.GetValue()) pending = std::move(completion);
}
