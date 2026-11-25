#include "WaveControl.hpp"
#include "control/Uniformity.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;

bool AnyBox::Shader::HasWave64Divergence(const IrProgram& program, const ShaderComputeInputInfo& input) {
    Control::Uniformity uniform(program, input);
    for (const auto& info : program.Metadata().blockInfo) {
        if (info.terminator.kind != TerminatorKind::ConditionalBranch || info.terminator.condition != BranchCondition::ScalarInstruction) continue;
        if (!info.condition || !uniform.Check(*info.condition)) return true;
    }
    return false;
}

void AnyBox::Shader::ValidateWave64Control(const IrProgram& program, const ShaderComputeInputInfo& input) {
    if (HasWave64Divergence(program, input)) throw std::runtime_error("Wave64 per-invocation control flow on a 32-lane subgroup is not implemented");
}

bool AnyBox::Shader::IsWaveUniform(const IrProgram& program, const IrValue& value, const ShaderComputeInputInfo& input) {
    return Control::Uniformity(program, input).Check(value);
}
