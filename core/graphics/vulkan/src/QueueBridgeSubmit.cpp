#include <anybox/vulkan/QueueBridge.hpp>
#include <anybox/platform/WindowsError.hpp>
#include <d3d12.h>
#include <algorithm>
#include <limits>

void AnyBox::Vulkan::QueueBridge::Submit(std::span<const VkCommandBuffer> commands) {
    Check();
    if (commands.size() > UINT32_MAX) throw std::length_error("Queue bridge command count exceeds Vulkan limits");
    for (auto command : commands) if (!command) throw std::invalid_argument("Queue bridge command buffer is null");
    if (value > std::numeric_limits<std::uint64_t>::max() - 4) throw std::overflow_error("Queue bridge fence values are exhausted");
    auto begin = value + 1;
    auto end = value + 2;
    auto semaphore = timeline->GetHandle();
    VkTimelineSemaphoreSubmitInfo values{};
    values.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    values.waitSemaphoreValueCount = values.signalSemaphoreValueCount = 1;
    values.pWaitSemaphoreValues = &begin;
    values.pSignalSemaphoreValues = &end;
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submission{};
    submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submission.pNext = &values;
    submission.waitSemaphoreCount = submission.signalSemaphoreCount = 1;
    submission.pWaitSemaphores = submission.pSignalSemaphores = &semaphore;
    submission.pWaitDstStageMask = &stage;
    submission.commandBufferCount = static_cast<std::uint32_t>(commands.size());
    submission.pCommandBuffers = commands.data();

    failed = true;
    Windows::CheckHResult(host.Signal(fence, begin), "Signal D3D12 queue bridge entry");
    lastHost = begin;
    context.Submit(queueIndex, submission);
    lastGpu = end;
    Windows::CheckHResult(host.Wait(fence, end), "Wait D3D12 queue bridge completion");
    value = end;
    failed = false;
}

bool AnyBox::Vulkan::QueueBridge::WaitIdle(std::uint64_t timeoutNanoseconds) const { return timeline->Wait(std::max(lastHost, lastGpu), timeoutNanoseconds); }
