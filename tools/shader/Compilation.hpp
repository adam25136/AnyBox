#pragma once

#include <anybox/shader/Artifact.hpp>
#include <anybox/shader/ComputeCompiler.hpp>

namespace AnyBox::Shader::Tools {
ComputeArtifact CompileDxil(std::span<const std::byte> code);
ComputeArtifact CompileNative(std::span<const std::byte> code);
void CheckArtifactGpu(const ComputeArtifact& artifact);
ComputeArtifact MakeArtifact(ComputeCompilation result, std::span<const BufferBinding> buffers, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderDigest& source);
}
