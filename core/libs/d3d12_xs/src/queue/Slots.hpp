#pragma once

#include <atomic>
#include <bit>
#include <cstdint>
#include <exception>
#include <stdexcept>

namespace AnyBox::Graphics {
class GpuQueueSlots {
public:
    std::uint32_t Acquire(std::uint32_t count) {
        if (count == 0 || count > 32) throw std::logic_error("Vulkan queue count cannot be represented by the device allocator");
        auto available = count == 32 ? UINT32_MAX : (1u << count) - 1;
        auto previous = used.load(std::memory_order_relaxed);
        for (;;) {
            auto free = available & ~previous;
            if (free == 0) throw std::runtime_error("No independent Vulkan queue is available");
            auto index = static_cast<std::uint32_t>(std::countr_zero(free));
            if (used.compare_exchange_weak(previous, previous | (1u << index), std::memory_order_acq_rel)) return index;
        }
    }
    void Release(std::uint32_t count, std::uint32_t index) noexcept {
        if (index >= count || (used.fetch_and(~(1u << index), std::memory_order_release) & (1u << index)) == 0) std::terminate();
    }

private:
    std::atomic<std::uint32_t> used{0};
};
}
