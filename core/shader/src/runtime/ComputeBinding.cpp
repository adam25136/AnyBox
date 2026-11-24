#include <anybox/shader/runtime/ComputeBinding.hpp>
#include <bit>

using namespace AnyBox::Shader::Runtime;

ComputeBinding::ComputeBinding(const ComputePipeline& pipeline, ComputeMemory& memory) : pipeline(pipeline), context(pipeline.GetContext()) {
    if (&memory.GetContext() != &context) throw std::invalid_argument("Compute pipeline and memory belong to different Vulkan devices");
    auto mask = pipeline.GetDescriptorMask();
    if (mask == 0) return;
    const auto& functions = context.GetFunctions();
    try {
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<std::uint32_t>(std::popcount(mask))};
        VkDescriptorPoolCreateInfo description{};
        description.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        description.maxSets = 1;
        description.poolSizeCount = 1;
        description.pPoolSizes = &size;
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
        std::array<VkDescriptorBufferInfo, 2> buffers{{{pages.GetHandle(), 0, pages.GetSize()}, {faults.GetHandle(), 0, faults.GetSize()}}};
        std::array<VkWriteDescriptorSet, 2> writes{};
        std::uint32_t count = 0;
        for (std::uint32_t index = 0; index < buffers.size(); ++index) {
            if ((mask & (1u << index)) == 0) continue;
            auto& write = writes[count++];
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = set;
            write.dstBinding = ComputePipeline::MemoryTableBinding + index;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.pBufferInfo = &buffers[index];
        }
        functions.updateDescriptorSets(context.GetDevice(), count, writes.data(), 0, nullptr);
    } catch (...) {
        if (pool) functions.destroyDescriptorPool(context.GetDevice(), pool, nullptr);
        throw;
    }
}

ComputeBinding::~ComputeBinding() { if (pool) context.GetFunctions().destroyDescriptorPool(context.GetDevice(), pool, nullptr); }

void ComputeBinding::Dispatch(VkCommandBuffer commands, std::span<const std::byte> constants, const std::array<std::uint32_t, 3>& groups) const { pipeline.GetPipeline().Dispatch(commands, constants, groups, GetSets()); }
