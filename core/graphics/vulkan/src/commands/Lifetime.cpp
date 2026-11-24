#include "State.hpp"
#include <limits>

using namespace AnyBox::Vulkan;

CommandArenaState::CommandArenaState(Context& context) : context(context) {
    VkCommandPoolCreateInfo description{};
    description.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    description.queueFamilyIndex = context.GetQueueFamily();
    Vulkan::Check(context.GetFunctions().createCommandPool(context.GetDevice(), &description, nullptr, &pool), "Create Vulkan command arena");
}

CommandArenaState::~CommandArenaState() {
    std::array<VkSemaphore, 32> semaphores;
    std::array<std::uint64_t, 32> values;
    std::uint32_t count = 0;
    for (const auto& submission : pending) {
        if (submission.GetValue() == 0) continue;
        semaphores[count] = submission.GetSemaphore();
        values[count++] = submission.GetValue();
    }
    if (count != 0) {
        VkSemaphoreWaitInfo wait{};
        wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        wait.semaphoreCount = count;
        wait.pSemaphores = semaphores.data();
        wait.pValues = values.data();
        auto result = context.GetFunctions().waitSemaphores(context.GetDevice(), &wait, std::numeric_limits<std::uint64_t>::max());
        if (result != VK_SUCCESS && result != VK_ERROR_DEVICE_LOST) std::terminate();
    }
    context.GetFunctions().destroyCommandPool(context.GetDevice(), pool, nullptr);
}

void CommandArenaState::Check(std::uint64_t expected) const {
    if (failed) throw std::runtime_error("Vulkan command arena has failed");
    if (expected != generation) throw std::logic_error("Vulkan command record was invalidated by an allocator reset");
}
