#include "Uniformity.hpp"
#include "LocalExpression.hpp"

using namespace ShaderRecompiler;
using AnyBox::Shader::Control::Uniformity;

std::optional<std::vector<const IrValue*>> Uniformity::classify(const IrValue& value) const {
    if (value.HasImmediate() || IsWaveLocalUniform(value, input)) return std::vector<const IrValue*>{};
    auto opcode = value.Opcode();
    if (opcode == IrOpcode::GetUserData) return std::vector<const IrValue*>{};
    if (opcode == IrOpcode::ReadFirstLane || opcode == IrOpcode::Ballot || (opcode == IrOpcode::ReadLane && value.ArgumentCount() == 2)) {
        if (!phiControlValid) return {};
        std::vector<const IrValue*> dependencies;
        if (opcode == IrOpcode::ReadLane) dependencies.push_back(value.Argument(1)->Resolve());
        return dependencies;
    }
    if (opcode == IrOpcode::GetBuiltin) {
        bool uniform = value.ArgumentCount() == 2 && value.Argument(0)->HasImmediate() && value.Argument(0)->ImmediateU32() == static_cast<std::uint32_t>(StageInputKind::WorkgroupId) && value.Argument(1)->HasImmediate() && value.Argument(1)->ImmediateU32() < 3;
        return uniform ? std::optional(std::vector<const IrValue*>{}) : std::nullopt;
    }
    bool phi = value.IsPhi();
    if (phi && (!phiControlValid || value.PhiBlockCount() != value.ArgumentCount())) return {};
    if ((!phi && !IsUniformArithmetic(opcode)) || value.ArgumentCount() == 0) return {};
    std::vector<const IrValue*> dependencies;
    for (auto* argument : value.Arguments()) dependencies.push_back(argument->Resolve());
    return dependencies;
}
