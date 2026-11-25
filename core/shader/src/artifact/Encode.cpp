#include "Binary.hpp"

using namespace AnyBox::Shader;
using namespace AnyBox::Shader::ArtifactFormat;

std::vector<std::byte> AnyBox::Shader::EncodeComputeArtifact(const ComputeArtifact& artifact) {
    Validate(artifact);
    Writer payload;
    for (auto value : {artifact.profile, artifact.runtimeVersion, artifact.bdaVersion}) payload.Word(value);
    for (auto value : artifact.threads) payload.Word(value);
    for (auto value : {artifact.waveSize, artifact.sharedMemoryBytes, artifact.pushConstantOffset, artifact.pushConstantSize, artifact.pushDataStart, artifact.stencilReference, artifact.memoryOffset, artifact.memoryOffsetCount, static_cast<std::uint32_t>(artifact.dispatchThreadLimit), artifact.runtimeImageCount, artifact.runtimeStateDwords, artifact.executionFlags, artifact.splitStateLanes, artifact.scratchPoolSlotDwords}) payload.Word(value);
    payload.Word(artifact.hostSubgroupSize);
    payload.Words(artifact.userRegisters);
    payload.Word(static_cast<std::uint32_t>(artifact.descriptors.size()));
    for (const auto& descriptor : artifact.descriptors) {
        payload.Word(descriptor.kind);
        payload.Words(descriptor.resources);
    }
    payload.Word(static_cast<std::uint32_t>(artifact.buffers.size()));
    for (const auto& buffer : artifact.buffers) {
        for (auto value : {static_cast<std::uint32_t>(buffer.resourceClass), buffer.rangeId, buffer.space, buffer.firstRegister, buffer.count, static_cast<std::uint32_t>(buffer.kind), buffer.stride, buffer.byteSize}) payload.Word(value);
        payload.Word(static_cast<std::uint32_t>(buffer.globallyCoherent) | (static_cast<std::uint32_t>(buffer.hasCounter) << 1) | (static_cast<std::uint32_t>(buffer.rasterizerOrdered) << 2));
    }
    payload.Words(artifact.code);
    auto bytes = payload.Take();
    if (bytes.size() > UINT32_MAX) throw std::length_error("Shader artifact payload exceeds its size field");
    Writer output;
    output.Bytes(Magic);
    output.Word(2);
    output.Word(static_cast<std::uint32_t>(bytes.size()));
    output.Bytes(artifact.sourceHash);
    output.Bytes(HashShader(bytes));
    output.Bytes(bytes);
    return output.Take();
}
