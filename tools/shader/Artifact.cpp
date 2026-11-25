#include "Compilation.hpp"
#include <RuntimeAbi.hpp>
#include <algorithm>

AnyBox::Shader::ComputeArtifact AnyBox::Shader::Tools::MakeArtifact(ComputeCompilation result, std::span<const BufferBinding> buffers, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderDigest& source) {
    static_assert(ShaderRecompiler::RuntimeAbi::Version == ComputeArtifact{}.runtimeVersion);
    ComputeArtifact artifact;
    artifact.sourceHash = source;
    std::copy(std::begin(input.threadsNum), std::end(input.threadsNum), artifact.threads.begin());
    artifact.waveSize = input.waveSize;
    artifact.hostSubgroupSize = input.hostSubgroupSize;
    artifact.sharedMemoryBytes = input.ldsSizeDwords * 4;
    artifact.pushConstantOffset = result.bindings.pushConstantOffsetBytes;
    artifact.pushConstantSize = result.bindings.pushConstantSizeBytes;
    auto& layout = result.bindings.layout;
    artifact.pushDataStart = layout.pushDataStartDword;
    artifact.stencilReference = layout.stencilReferenceDword;
    artifact.memoryOffset = layout.memoryOffsetDword;
    artifact.memoryOffsetCount = layout.memoryOffsetCount;
    artifact.dispatchThreadLimit = layout.dispatchThreadLimit;
    artifact.runtimeImageCount = layout.runtimeImageCount;
    artifact.runtimeStateDwords = layout.runtimeStateDwords;
    artifact.userRegisters = std::move(layout.userDataRegisters);
    for (auto& descriptor : layout.descriptors) artifact.descriptors.push_back({static_cast<std::uint32_t>(descriptor.kind), std::move(descriptor.resources)});
    artifact.executionFlags = static_cast<std::uint32_t>(result.info.fullSubgroups) | (static_cast<std::uint32_t>(result.info.functionLdsBuffer) << 1) | (static_cast<std::uint32_t>(result.info.meshWindowBuffer) << 2) | (static_cast<std::uint32_t>(result.info.anySubgroupSize) << 3);
    artifact.splitStateLanes = result.info.splitStateLanes;
    artifact.scratchPoolSlotDwords = result.info.scratchPoolSlotDwords;
    artifact.buffers.assign(buffers.begin(), buffers.end());
    artifact.code = std::move(result.code);
    return artifact;
}
