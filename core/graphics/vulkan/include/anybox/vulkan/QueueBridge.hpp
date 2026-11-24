#pragma once

#include "Timeline.hpp"
#include "Submission.hpp"
#include <span>

struct ID3D12CommandQueue;
struct ID3D12Fence;

namespace AnyBox::Vulkan {
class QueueBridge {
public:
    QueueBridge(Context& context, ID3D12CommandQueue& host, std::uint32_t queueIndex);
    ~QueueBridge();
    QueueBridge(const QueueBridge&) = delete;
    QueueBridge& operator=(const QueueBridge&) = delete;
    void Check() const;
    void Submit(std::span<const VkCommandBuffer> commands);
    bool WaitIdle(std::uint64_t timeoutNanoseconds) const;
    Submission GetCompletion() const noexcept { return {timeline, lastGpu}; }
    Context& GetContext() const noexcept { return context; }

private:
    Context& context;
    ID3D12CommandQueue& host;
    std::uint32_t queueIndex;
    ID3D12Fence* fence = nullptr;
    std::shared_ptr<Timeline> timeline;
    std::uint64_t value = 0;
    std::uint64_t lastHost = 0;
    std::uint64_t lastGpu = 0;
    bool failed = false;
};
}
