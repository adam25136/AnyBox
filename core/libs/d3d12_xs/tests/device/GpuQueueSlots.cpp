#include "GpuQueueFixture.hpp"

void AnyBox::Graphics::Tests::CheckGpuQueueSlots(Device* device) {
    auto count = device->GetGpuContext().GetQueueCount();
    std::vector<QueueOwner> queues;
    for (std::uint32_t index = 0; index < count; ++index) {
        auto queue = CreateGpuQueue(device);
        queue->SubmitGpu({});
        queues.push_back(std::move(queue));
    }
    auto waiting = CreateGpuQueue(device);
    Reject<std::runtime_error>([&] { waiting->SubmitGpu({}); });
    queues.front().reset();
    waiting->SubmitGpu({});
}
