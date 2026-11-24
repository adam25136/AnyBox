#pragma once

#include "../../pipeline/State.hpp"
#include "../../pipeline/Offline.hpp"
#include <anybox/shader/runtime/ComputeBinding.hpp>
#include <unordered_map>

namespace AnyBox::Graphics {
struct GpuEpochBinding {
    GpuEpochBinding(PipelineState& pipeline, Shader::Runtime::ComputeMemory& memory);
    std::unique_ptr<PipelineState, decltype(&PipelineState::Release)> pipeline;
    Shader::Runtime::ComputeBinding binding;
};

struct GpuEpoch {
    static std::shared_ptr<GpuEpoch> Create(Device& device);
    GpuEpochBinding& GetBinding(PipelineState& pipeline);
    std::shared_ptr<Shader::Runtime::ComputeMemory> memory;
    std::uint64_t bufferGeneration;
    std::uint64_t tableGeneration;
    std::unordered_map<PipelineState*, std::unique_ptr<GpuEpochBinding>> bindings;
    PipelineState* previous = nullptr;
    GpuEpochBinding* selected = nullptr;
};
}
