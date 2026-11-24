#include "Queue.hpp"
#include <anybox/vulkan/QueueBridge.hpp>

std::unique_lock<std::mutex> AnyBox::Graphics::Queue::lockCommands() {
    std::unique_lock lock(commandMutex);
    if (gpu) gpu->Check();
    return lock;
}

void AnyBox::Graphics::Queue::SubmitGpu(std::span<const VkCommandBuffer> commands) {
    auto lock = lockCommands();
    if (!gpu) {
        auto index = device->AcquireGpuQueue();
        try {
            auto created = std::make_unique<Vulkan::QueueBridge>(device->GetGpuContext(), *host, index);
            gpuQueueIndex = index;
            gpu = created.release();
        } catch (...) {
            device->ReleaseGpuQueue(index);
            throw;
        }
    }
    gpu->Submit(commands);
}
