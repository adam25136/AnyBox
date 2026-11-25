#include "Plan.hpp"
#include <stdexcept>

namespace {
using namespace ShaderRecompiler;

bool Pure(IrOpcode opcode) {
    switch (opcode) {
        case IrOpcode::Identity:
        case IrOpcode::GetBuiltin:
        case IrOpcode::GetUserData:
        case IrOpcode::GetAddressResource:
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
        case IrOpcode::ULessThan32:
        case IrOpcode::ULessThan64:
        case IrOpcode::UGreaterThan32:
        case IrOpcode::UGreaterThan64:
        case IrOpcode::LogicalAnd:
        case IrOpcode::LogicalOr:
        case IrOpcode::LogicalXor:
        case IrOpcode::LogicalNot:
        case IrOpcode::SelectU1:
        case IrOpcode::SelectU32:
        case IrOpcode::SelectF32:
        case IrOpcode::BitCastF32U32:
        case IrOpcode::BitCastU32F32:
        case IrOpcode::Branch: return true;
        default: return false;
    }
}

std::optional<std::size_t> Activity(const IrValue& value) {
    if (value.Opcode() == IrOpcode::DescriptorFault) return 0;
    if (AddressOpcodeInfoOf(value.Opcode()).access != AddressAccess::None || SharedAccessOf(value.Opcode()) != SharedAccess::None) {
        if (value.ArgumentCount() == 0) throw std::logic_error("DXIL predicated memory operation has no activity operand");
        return value.ArgumentCount() - 1;
    }
    return {};
}
}

bool AnyBox::Shader::Dxil::Flow::Predication::CanMask(const IrBlock& block) {
    for (auto* instruction : block.Instructions()) {
        auto activity = Activity(*instruction);
        if (activity) {
            if (*activity >= instruction->ArgumentCount() || instruction->Argument(*activity)->Type() != IrType::Bool) return false;
        } else if (!Pure(instruction->Opcode())) return false;
    }
    return true;
}

void AnyBox::Shader::Dxil::Flow::Predication::Mask(IrProgram& program, IrBlock& block, IrValue& active) {
    std::vector<IrValue*> original(block.Instructions().begin(), block.Instructions().end());
    for (auto* instruction : original) {
        auto index = Activity(*instruction);
        if (!index) continue;
        auto* predicate = instruction->Argument(*index)->Resolve();
        if (predicate->HasImmediate()) {
            if (predicate->ImmediateBool()) instruction->ReplaceArgument(*index, &active);
            continue;
        }
        auto& combined = program.CreateValue(IrOpcode::LogicalAnd, IrType::Bool);
        combined.AddArgument(predicate);
        combined.AddArgument(&active);
        block.InsertInstructionBefore(instruction, &combined);
        instruction->ReplaceArgument(*index, &combined);
    }
}
