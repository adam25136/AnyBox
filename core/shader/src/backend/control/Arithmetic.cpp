#include "Uniformity.hpp"

bool AnyBox::Shader::Control::IsUniformArithmetic(ShaderRecompiler::IrOpcode opcode) {
    using ShaderRecompiler::IrOpcode;
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
        case IrOpcode::FindILsb32:
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
