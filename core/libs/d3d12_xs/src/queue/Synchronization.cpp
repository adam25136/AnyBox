#include "Queue.hpp"
#include "../fence/Fence.hpp"

HRESULT AnyBox::Graphics::Queue::Signal(Queue* self, void* fence, std::uint64_t value) {
    auto lock = self->lockCommands();
    auto result = self->host->Signal(Fence::From(fence)->GetHost(), value);
    CheckGraphicsResult(result, "ID3D12CommandQueue::Signal");
    return result;
}

HRESULT AnyBox::Graphics::Queue::Wait(Queue* self, void* fence, std::uint64_t value) {
    auto lock = self->lockCommands();
    auto result = self->host->Wait(Fence::From(fence)->GetHost(), value);
    CheckGraphicsResult(result, "ID3D12CommandQueue::Wait");
    return result;
}
