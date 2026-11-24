#pragma once

#include "ComputeMemory.hpp"
#include "ComputePipeline.hpp"

namespace AnyBox::Shader::Runtime {
class ComputeBinding {
public:
    ComputeBinding(const ComputePipeline& pipeline, ComputeMemory& memory);
    ~ComputeBinding();
    ComputeBinding(const ComputeBinding&) = delete;
    ComputeBinding& operator=(const ComputeBinding&) = delete;
    std::span<const VkDescriptorSet> GetSets() const noexcept { return {&set, set ? 1u : 0u}; }
    void Dispatch(VkCommandBuffer commands, std::span<const std::byte> constants, const std::array<std::uint32_t, 3>& groups) const;

private:
    const ComputePipeline& pipeline;
    Vulkan::Context& context;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
};
}
