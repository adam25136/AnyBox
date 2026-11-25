#pragma once

#include <anybox/shader/runtime/ComputeMemory.hpp>
#include <anybox/shader/runtime/ComputePipeline.hpp>

namespace AnyBox::Shader::Runtime {
class ComputeDescriptors {
public:
    ComputeDescriptors(const ComputePipeline& pipeline, ComputeMemory& memory, const Vulkan::Buffer* data = nullptr);
    ~ComputeDescriptors();
    ComputeDescriptors(const ComputeDescriptors&) = delete;
    ComputeDescriptors& operator=(const ComputeDescriptors&) = delete;
    std::span<const VkDescriptorSet> GetSets() const noexcept { return {&set, set ? 1u : 0u}; }

private:
    Vulkan::Context& context;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
};
}
