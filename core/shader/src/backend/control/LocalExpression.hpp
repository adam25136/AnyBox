#pragma once

#include <IntermediateRepresentation/IrValue.hpp>
#include <Optimization/ShaderStageInputInfo.hpp>
#include <optional>
#include <unordered_map>

namespace AnyBox::Shader::Control {
class LocalExpression {
public:
    LocalExpression(const ShaderRecompiler::ShaderComputeInputInfo& inputInfo, std::uint32_t localIndex) : input(inputInfo), invocation(localIndex) {}
    std::optional<std::uint32_t> Evaluate(const ShaderRecompiler::IrValue& expression);

private:
    std::optional<std::uint32_t> evaluate(const ShaderRecompiler::IrValue& expression);
    const ShaderRecompiler::ShaderComputeInputInfo& input;
    std::uint32_t invocation;
    std::unordered_map<const ShaderRecompiler::IrValue*, std::optional<std::uint32_t>> known;
};

bool IsWaveLocalUniform(const ShaderRecompiler::IrValue& expression, const ShaderRecompiler::ShaderComputeInputInfo& input);
}
