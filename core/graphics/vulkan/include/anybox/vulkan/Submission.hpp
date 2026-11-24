#pragma once

#include "Timeline.hpp"
#include <utility>

namespace AnyBox::Vulkan {
class Submission {
public:
    Submission() = default;
    Submission(std::shared_ptr<const Timeline> timeline, std::uint64_t value) noexcept : timeline(std::move(timeline)), value(value) {}
    const Timeline* GetSource() const noexcept { return timeline.get(); }
    std::uint64_t GetValue() const noexcept { return value; }
    bool IsComplete() const { return value == 0 || (timeline && timeline->GetValue() >= value); }
    VkSemaphore GetSemaphore() const noexcept { return timeline ? timeline->GetHandle() : VK_NULL_HANDLE; }

private:
    std::shared_ptr<const Timeline> timeline;
    std::uint64_t value = 0;
};
}
