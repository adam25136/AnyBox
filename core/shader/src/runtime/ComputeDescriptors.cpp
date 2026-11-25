#include "ComputeDescriptors.hpp"
#include <bit>

using namespace AnyBox::Shader::Runtime;

ComputeDescriptors::ComputeDescriptors(const ComputePipeline& pipeline, ComputeMemory& memory, const AnyBox::Vulkan::Buffer* data) : context(pipeline.GetContext()) {
    if (&memory.GetContext() != &context) throw std::invalid_argument("Compute pipeline and memory belong to different Vulkan devices");
    auto mask = pipeline.GetDescriptorMask();
    if (static_cast<bool>(mask & 8) != static_cast<bool>(data)) throw std::invalid_argument("Compute shader data buffer does not match the pipeline layout");
    if (mask == 0) return;
    const auto& functions = context.GetFunctions();
    try {
        std::array<VkDescriptorPoolSize, 2> sizes{};
        std::uint32_t sizeCount = 0;
        if (mask & 3) sizes[sizeCount++] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<std::uint32_t>(std::popcount(mask & 3))};
        if (data) sizes[sizeCount++] = {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1};
        VkDescriptorPoolCreateInfo description{};
        description.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        description.maxSets = 1;
        description.poolSizeCount = sizeCount;
        description.pPoolSizes = sizes.data();
        Vulkan::Check(functions.createDescriptorPool(context.GetDevice(), &description, nullptr, &pool), "Create compute metadata descriptor pool");
        auto layout = pipeline.GetDescriptorLayout();
        VkDescriptorSetAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocation.descriptorPool = pool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &layout;
        Vulkan::Check(functions.allocateDescriptorSets(context.GetDevice(), &allocation, &set), "Allocate compute metadata descriptor set");

        const auto& pages = memory.GetTableBuffer();
        const auto& faults = memory.GetFaultBuffer();
        std::array<VkDescriptorBufferInfo, 4> buffers{{{pages.GetHandle(), 0, pages.GetSize()}, {faults.GetHandle(), 0, faults.GetSize()}, {}, {}}};
        if (data) buffers[3] = {data->GetHandle(), 0, pipeline.GetShaderDataSize()};
        std::array<VkWriteDescriptorSet, 3> writes{};
        std::uint32_t count = 0;
        for (std::uint32_t index = 0; index < buffers.size(); ++index) {
            if ((mask & (1u << index)) == 0) continue;
            auto& write = writes[count++];
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = set;
            write.dstBinding = ComputePipeline::MemoryTableBinding + index;
            write.descriptorCount = 1;
            write.descriptorType = index == 3 ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.pBufferInfo = &buffers[index];
        }
        functions.updateDescriptorSets(context.GetDevice(), count, writes.data(), 0, nullptr);
    } catch (...) {
        if (pool) functions.destroyDescriptorPool(context.GetDevice(), pool, nullptr);
        throw;
    }
}

ComputeDescriptors::~ComputeDescriptors() { if (pool) context.GetFunctions().destroyDescriptorPool(context.GetDevice(), pool, nullptr); }

