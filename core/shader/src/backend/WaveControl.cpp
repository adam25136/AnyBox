#include "WaveControl.hpp"
#include "control/Uniformity.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;

void AnyBox::Shader::ValidateWave64Control(const IrProgram& program, const ShaderComputeInputInfo& input) {
    Control::Uniformity uniform(program, input);
    for (const auto& info : program.Metadata().blockInfo) {
        if (info.terminator.kind != TerminatorKind::ConditionalBranch || info.terminator.condition != BranchCondition::ScalarInstruction) continue;
        if (!info.condition || !uniform.Check(*info.condition)) throw std::runtime_error("Wave64 per-invocation control flow on a 32-lane subgroup is not implemented");
    }
}

bool AnyBox::Shader::IsWaveUniform(const IrProgram& program, const IrValue& value, const ShaderComputeInputInfo& input) {
    return Control::Uniformity(program, input).Check(value);
}
