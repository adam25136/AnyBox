#include <anybox/shader/runtime/ComputeMemory.hpp>

using namespace AnyBox::Shader::Runtime;

void ComputeMemory::Begin(VkCommandBuffer commands) const {
    if (!commands) throw std::invalid_argument("Compute metadata command buffer is null");
    const auto& functions = context.GetFunctions();
    if (clearOnBegin) {
        VkBufferMemoryBarrier reuse{};
        reuse.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        reuse.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        reuse.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        reuse.srcQueueFamilyIndex = reuse.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        reuse.buffer = faults->GetHandle();
        reuse.size = VK_WHOLE_SIZE;
        functions.pipelineBarrier(commands, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &reuse, 0, nullptr);
        functions.fillBuffer(commands, faults->GetHandle(), 0, VK_WHOLE_SIZE, 0);
    }

    VkMemoryBarrier ready{};
    ready.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    ready.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    ready.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    functions.pipelineBarrier(commands, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &ready, 0, nullptr, 0, nullptr);
}

void ComputeMemory::End(VkCommandBuffer commands) const {
    if (!commands) throw std::invalid_argument("Compute metadata command buffer is null");
    VkBufferMemoryBarrier visible{};
    visible.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    visible.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    visible.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    visible.srcQueueFamilyIndex = visible.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    visible.buffer = faults->GetHandle();
    visible.size = VK_WHOLE_SIZE;
    context.GetFunctions().pipelineBarrier(commands, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &visible, 0, nullptr);
}
