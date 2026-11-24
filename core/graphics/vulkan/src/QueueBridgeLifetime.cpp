#include <anybox/vulkan/QueueBridge.hpp>
#include <d3d12.h>
#include <algorithm>
#include <limits>

AnyBox::Vulkan::QueueBridge::~QueueBridge() {
    auto target = std::max(lastHost, lastGpu);
    if (target != 0) {
        ++target;
        if (FAILED(host.Signal(fence, target))) std::terminate();
        auto semaphore = timeline->GetHandle();
        VkSemaphoreWaitInfo wait{};
        wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        wait.semaphoreCount = 1;
        wait.pSemaphores = &semaphore;
        wait.pValues = &target;
        auto result = context.GetFunctions().waitSemaphores(context.GetDevice(), &wait, std::numeric_limits<std::uint64_t>::max());
        if (result != VK_SUCCESS && result != VK_ERROR_DEVICE_LOST) std::terminate();
    }
    timeline.reset();
    fence->Release();
    host.Release();
}
