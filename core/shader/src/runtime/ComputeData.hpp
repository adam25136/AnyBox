#pragma once

#include "ComputeDescriptors.hpp"

namespace AnyBox::Shader::Runtime {
class ComputeData {
public:
    ComputeData(const ComputePipeline& pipeline, ComputeMemory& memory);
    std::span<const VkDescriptorSet> GetSets() const noexcept { return chunks.back()->descriptors.GetSets(); }
    std::uint32_t Write(std::span<const std::byte> values);

private:
    struct Chunk {
        Chunk(const ComputePipeline& pipeline, ComputeMemory& memory, VkDeviceSize capacity);
        Vulkan::Buffer buffer;
        ComputeDescriptors descriptors;
        std::span<std::byte> mapped;
        VkDeviceSize used = 0;
    };
    const ComputePipeline& pipeline;
    ComputeMemory& memory;
    VkDeviceSize stride;
    VkDeviceSize capacity;
    std::vector<std::unique_ptr<Chunk>> chunks;
};
}
