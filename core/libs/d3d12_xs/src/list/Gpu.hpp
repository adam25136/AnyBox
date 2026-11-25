#pragma once

#include "Compute.hpp"
#include <anybox/vulkan/CommandArena.hpp>
#include <anybox/shader/runtime/RootResources.hpp>

namespace AnyBox::Graphics {
class Allocator;
class PipelineState;
struct GpuEpoch;

class GpuRecording {
public:
    GpuRecording(Device& device, Allocator& allocator);
    void Dispatch(PipelineState& pipeline, const ComputeState& compute, const std::array<std::uint32_t, 3>& groups);
    void Barrier();
    void Close();
    void Check() const;
    void Fail() noexcept { failed = true; }
    Vulkan::CommandArena& GetArena() const noexcept { return arena; }
    const Vulkan::CommandRecord& GetRecord() const noexcept { return record; }

private:
    void updateMemory();
    Device& device;
    Vulkan::CommandArena& arena;
    Vulkan::CommandRecord record;
    Shader::Runtime::RootResources resources;
    std::shared_ptr<GpuEpoch> epoch;
    std::vector<std::uint32_t> externalArguments;
    bool failed = false;
};
}
