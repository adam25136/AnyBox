#include "../Gpu.hpp"
#include "../../allocator/Allocator.hpp"
#include "Epoch.hpp"

using namespace AnyBox::Graphics;

GpuRecording::GpuRecording(Device& device, Allocator& allocator) : device(device), arena(allocator.GetGpuArena()), record(arena.Begin()), resources(device.GetBufferAddresses(), device.GetDescriptorAddresses(), device.GetNullConstantAddress()) {}

void GpuRecording::Check() const { if (failed) throw std::runtime_error("Vulkan command recording has failed"); }

void GpuRecording::updateMemory() {
    if (epoch && epoch->bufferGeneration == device.GetBufferAddresses().GetGeneration() && epoch->tableGeneration == device.GetDescriptorAddresses().GetGeneration()) return;
    auto next = GpuEpoch::Create(device);
    record.Retain(next);
    record.Verify(next->memory);
    next->memory->Begin(record.GetCommands());
    epoch = std::move(next);
}

void GpuRecording::Dispatch(PipelineState& pipeline, const ComputeState& compute, const std::array<std::uint32_t, 3>& groups) {
    Check();
    const auto& offline = *pipeline.GetOffline();
    const auto& writer = offline.GetArgumentWriter();
    std::array<std::uint32_t, 32> constants{};
    resources.Reset();
    if (writer.GetWordCount() != 0) {
        auto start = offline.GetPushDataStart();
        if (start > constants.size() || writer.GetWordCount() > constants.size() - start) throw std::runtime_error("Compute root data exceeds the inline push constant capacity");
        writer.Write(compute.GetArguments(), resources, std::span(constants).subspan(start, writer.GetWordCount()));
    }
    updateMemory();
    auto& binding = epoch->GetBinding(pipeline).binding;
    binding.Dispatch(record.GetCommands(), std::as_bytes(std::span(constants)).first(offline.GetPushConstantSize()), groups);
    resources.Reset();
}

void GpuRecording::Barrier() {
    Check();
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    device.GetGpuContext().GetFunctions().pipelineBarrier(record.GetCommands(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void GpuRecording::Close() {
    Check();
    VkMemoryBarrier visible{};
    visible.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    visible.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    visible.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    device.GetGpuContext().GetFunctions().pipelineBarrier(record.GetCommands(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &visible, 0, nullptr, 0, nullptr);
    record.Close();
}
