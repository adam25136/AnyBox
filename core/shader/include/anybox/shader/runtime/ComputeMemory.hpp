#pragma once

#include "MemoryTable.hpp"
#include <anybox/vulkan/Buffer.hpp>
#include <anybox/vulkan/ICompletionCheck.hpp>

namespace AnyBox::Shader::Runtime {
enum class FaultLifetime { Submission, Recording };

class ComputeMemory : public Vulkan::ICompletionCheck {
public:
    ComputeMemory(Vulkan::Context& context, MemoryTable table, FaultLifetime faultLifetime = FaultLifetime::Submission);
    ComputeMemory(const ComputeMemory&) = delete;
    ComputeMemory& operator=(const ComputeMemory&) = delete;
    Vulkan::Context& GetContext() const noexcept { return context; }
    const Vulkan::Buffer& GetTableBuffer() const noexcept { return *pages; }
    Vulkan::Buffer& GetFaultBuffer() noexcept { return *faults; }
    void Begin(VkCommandBuffer commands) const;
    void End(VkCommandBuffer commands) const;
    void CheckFault();
    void CheckCompletion() override { CheckFault(); }

private:
    Vulkan::Context& context;
    MemoryTable table;
    std::unique_ptr<Vulkan::Buffer> pages;
    std::unique_ptr<Vulkan::Buffer> faults;
    bool clearOnBegin;
};
}
