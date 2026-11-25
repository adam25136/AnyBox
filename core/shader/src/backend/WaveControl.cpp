#include "WaveControl.hpp"
#include "control/LocalExpression.hpp"
#include <IntermediateRepresentation/IrProgram.hpp>
#include <unordered_map>
#include <stdexcept>

namespace {
using namespace ShaderRecompiler;

bool Arithmetic(IrOpcode opcode) {
    switch (opcode) {
        case IrOpcode::IAdd32:
        case IrOpcode::ISub32:
        case IrOpcode::IMul32:
        case IrOpcode::IAdd64:
        case IrOpcode::ISub64:
        case IrOpcode::IMul64:
        case IrOpcode::ShiftLeftLogical32:
        case IrOpcode::ShiftLeftLogical64:
        case IrOpcode::ShiftRightLogical32:
        case IrOpcode::ShiftRightLogical64:
        case IrOpcode::ShiftRightArithmetic32:
        case IrOpcode::ShiftRightArithmetic64:
        case IrOpcode::BitwiseAnd32:
        case IrOpcode::BitwiseAnd64:
        case IrOpcode::BitwiseOr32:
        case IrOpcode::BitwiseXor32:
        case IrOpcode::BitwiseNot32:
        case IrOpcode::CompositeConstructU64:
        case IrOpcode::CompositeExtractU64:
        case IrOpcode::CompositeExtractU32x2:
        case IrOpcode::CompositeExtractU32x3:
        case IrOpcode::CompositeExtractU32x4:
        case IrOpcode::IEqual32:
        case IrOpcode::IEqual64:
        case IrOpcode::INotEqual32:
        case IrOpcode::INotEqual64:
        case IrOpcode::SLessThan32:
        case IrOpcode::SLessThan64:
        case IrOpcode::ULessThan32:
        case IrOpcode::ULessThan64:
        case IrOpcode::UGreaterThan32:
        case IrOpcode::UGreaterThan64:
        case IrOpcode::LogicalAnd:
        case IrOpcode::LogicalOr:
        case IrOpcode::LogicalXor:
        case IrOpcode::LogicalNot:
        case IrOpcode::SelectU1:
        case IrOpcode::SelectU32: return true;
        default: return false;
    }
}

class Uniformity {
public:
    explicit Uniformity(const ShaderComputeInputInfo& inputInfo) : input(inputInfo) {}
    bool Check(const IrValue& expression) {
        const auto& value = *expression.Resolve();
        if (value.HasImmediate()) return true;
        if (auto found = known.find(&value); found != known.end()) return found->second;
        known.emplace(&value, false);
        if (AnyBox::Shader::Control::IsWaveLocalUniform(value, input)) return known[&value] = true;
        auto opcode = value.Opcode();
        if (opcode == IrOpcode::ReadLane && value.ArgumentCount() == 2 && Check(*value.Argument(1))) return known[&value] = true;
        if (opcode == IrOpcode::GetUserData || opcode == IrOpcode::ReadFirstLane || opcode == IrOpcode::Ballot) return known[&value] = true;
        if (opcode == IrOpcode::GetBuiltin) {
            bool uniform = value.ArgumentCount() == 2 && value.Argument(0)->HasImmediate() && value.Argument(0)->ImmediateU32() == static_cast<std::uint32_t>(StageInputKind::WorkgroupId) && value.Argument(1)->HasImmediate() && value.Argument(1)->ImmediateU32() < 3;
            return known[&value] = uniform;
        }
        if (!Arithmetic(opcode) || value.ArgumentCount() == 0) return false;
        for (auto* argument : value.Arguments()) if (!Check(*argument)) return false;
        return known[&value] = true;
    }

private:
    const ShaderComputeInputInfo& input;
    std::unordered_map<const IrValue*, bool> known;
};
}

void AnyBox::Shader::ValidateWave64Control(const ShaderRecompiler::IrProgram& program, const ShaderComputeInputInfo& input) {
    Uniformity uniform(input);
    for (const auto& info : program.Metadata().blockInfo) {
        if (info.terminator.kind != TerminatorKind::ConditionalBranch || info.terminator.condition != BranchCondition::ScalarInstruction) continue;
        if (!info.condition || !uniform.Check(*info.condition)) throw std::runtime_error("Wave64 per-invocation control flow on a 32-lane subgroup is not implemented");
    }
}

bool AnyBox::Shader::IsWaveUniform(const IrValue& value, const ShaderComputeInputInfo& input) {
    return Uniformity(input).Check(value);
}
