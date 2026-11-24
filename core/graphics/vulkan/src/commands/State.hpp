#pragma once

#include <anybox/vulkan/CommandArena.hpp>

namespace AnyBox::Vulkan {
struct CommandArenaState {
    explicit CommandArenaState(Context& context);
    ~CommandArenaState();
    void Check(std::uint64_t expected) const;
    Context& context;
    std::mutex mutex;
    VkCommandPool pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> buffers;
    std::vector<std::shared_ptr<void>> resources;
    std::array<Submission, 32> pending;
    std::uint64_t generation = 1;
    std::size_t cursor = 0;
    bool recording = false;
    bool failed = false;
};
}
