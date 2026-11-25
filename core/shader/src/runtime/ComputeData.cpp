#include "ComputeData.hpp"
#include <algorithm>
#include <cstring>

using namespace AnyBox::Shader::Runtime;

ComputeData::Chunk::Chunk(const ComputePipeline& pipeline, ComputeMemory& memory, VkDeviceSize capacity) : buffer(Vulkan::Buffer::Allocate(pipeline.GetContext(), capacity, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)), descriptors(pipeline, memory, &buffer), mapped(buffer.Map()) {}

ComputeData::ComputeData(const ComputePipeline& pipeline, ComputeMemory& memory) : pipeline(pipeline), memory(memory) {
    const auto& limits = pipeline.GetContext().GetProperties().limits;
    auto alignment = std::max<VkDeviceSize>(4, limits.minStorageBufferOffsetAlignment);
    stride = (pipeline.GetShaderDataSize() + alignment - 1) / alignment * alignment;
    if (stride == 0 || stride > UINT32_MAX) throw std::invalid_argument("Compute shader data stride exceeds the dynamic offset capacity");
    capacity = std::max<VkDeviceSize>(65536, stride);
    chunks.push_back(std::make_unique<Chunk>(pipeline, memory, capacity));
}

std::uint32_t ComputeData::Write(std::span<const std::byte> values) {
    if (values.size() != pipeline.GetShaderDataSize()) throw std::invalid_argument("Compute external shader data size differs from its pipeline layout");
    if (stride > capacity - chunks.back()->used) chunks.push_back(std::make_unique<Chunk>(pipeline, memory, capacity));
    auto& chunk = *chunks.back();
    auto offset = chunk.used;
    std::memcpy(chunk.mapped.data() + offset, values.data(), values.size());
    chunk.used += stride;
    return static_cast<std::uint32_t>(offset);
}
