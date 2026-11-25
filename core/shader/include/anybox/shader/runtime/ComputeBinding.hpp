#pragma once

#include "ComputeMemory.hpp"
#include "ComputePipeline.hpp"

namespace AnyBox::Shader::Runtime {
class ComputeData;
class ComputeDescriptors;
class ComputeBinding {
public:
    ComputeBinding(const ComputePipeline& pipeline, ComputeMemory& memory);
    ~ComputeBinding();
    ComputeBinding(const ComputeBinding&) = delete;
    ComputeBinding& operator=(const ComputeBinding&) = delete;
    std::span<const VkDescriptorSet> GetSets() const noexcept;
    void Dispatch(VkCommandBuffer commands, std::span<const std::byte> constants, const std::array<std::uint32_t, 3>& groups, std::span<const std::byte> externalData = {}) const;

private:
    const ComputePipeline& pipeline;
    std::unique_ptr<ComputeData> data;
    std::unique_ptr<ComputeDescriptors> descriptors;
};
}
