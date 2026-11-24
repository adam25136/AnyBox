#pragma once

#include "../root/Signature.hpp"
#include <anybox/shader/RootArgumentWriter.hpp>
#include <anybox/shader/runtime/ComputePipeline.hpp>

namespace AnyBox::Graphics {
class OfflinePipeline {
public:
    OfflinePipeline(Device& device, const RootSignature& root, Shader::ComputeArtifact artifact) : plan(artifact.buffers, root.GetLayout()), writer(plan, artifact.userRegisters), pipeline(device.GetGpuContext(), artifact), pushConstantSize(artifact.pushConstantSize), pushDataStart(artifact.pushDataStart), volatileTables(writer.RequiresVolatileTables(root.GetLayout())) {}
    const Shader::Runtime::ComputePipeline& GetPipeline() const noexcept { return pipeline; }
    const Shader::RootArgumentWriter& GetArgumentWriter() const noexcept { return writer; }
    std::uint32_t GetPushConstantSize() const noexcept { return pushConstantSize; }
    std::uint32_t GetPushDataStart() const noexcept { return pushDataStart; }
    bool HasVolatileTables() const noexcept { return volatileTables; }

private:
    Shader::RootArgumentPlan plan;
    Shader::RootArgumentWriter writer;
    Shader::Runtime::ComputePipeline pipeline;
    std::uint32_t pushConstantSize;
    std::uint32_t pushDataStart;
    bool volatileTables;
};

Shader::ComputeArtifact LoadPreparedCompute(std::span<const std::byte> code);
bool RequiresPreparedCompute(Device& device, std::span<const std::byte> code);
}
