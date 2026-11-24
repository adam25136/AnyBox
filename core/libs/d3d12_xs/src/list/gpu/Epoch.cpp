#include "Epoch.hpp"

using namespace AnyBox::Graphics;

namespace {
auto Retain(PipelineState& pipeline) {
    PipelineState::AddRef(&pipeline);
    return std::unique_ptr<PipelineState, decltype(&PipelineState::Release)>(&pipeline, &PipelineState::Release);
}
}

GpuEpochBinding::GpuEpochBinding(PipelineState& pipeline, Shader::Runtime::ComputeMemory& memory) : pipeline(Retain(pipeline)), binding(pipeline.GetOffline()->GetPipeline(), memory) {}

std::shared_ptr<GpuEpoch> GpuEpoch::Create(Device& device) {
    auto buffers = device.GetBufferAddresses().Capture();
    auto tables = device.GetDescriptorAddresses().Capture();
    std::vector<Shader::Runtime::MemoryBinding> mappings;
    mappings.reserve(buffers.mappings.size() + tables.mappings.size());
    for (auto& mapping : buffers.mappings) mappings.push_back({std::move(mapping)});
    for (auto& mapping : tables.mappings) mappings.push_back({std::move(mapping)});
    auto epoch = std::make_shared<GpuEpoch>();
    epoch->bufferGeneration = buffers.generation;
    epoch->tableGeneration = tables.generation;
    epoch->memory = std::make_shared<Shader::Runtime::ComputeMemory>(device.GetGpuContext(), Shader::Runtime::MemoryTable(mappings), Shader::Runtime::FaultLifetime::Recording);
    return epoch;
}

GpuEpochBinding& GpuEpoch::GetBinding(PipelineState& pipeline) {
    if (previous == &pipeline) return *selected;
    auto found = bindings.find(&pipeline);
    if (found == bindings.end()) found = bindings.emplace(&pipeline, std::make_unique<GpuEpochBinding>(pipeline, *memory)).first;
    previous = &pipeline;
    selected = found->second.get();
    return *selected;
}
