#include "Scalar.hpp"

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::floatBinary(const LLVMBC::BinaryOperator& instruction) {
    auto& left = Get(*instruction.getOperand(0));
    auto& right = Get(*instruction.getOperand(1));
    if (left.Type() != IrType::F32 || right.Type() != IrType::F32) throw std::runtime_error("DXIL floating binary operands must be float32");
    IrOpcode opcode;
    switch (instruction.getOpcode()) {
        case LLVMBC::Instruction::FAdd: opcode = IrOpcode::FPAdd32; break;
        case LLVMBC::Instruction::FSub: opcode = IrOpcode::FPSub32; break;
        case LLVMBC::Instruction::FMul: opcode = IrOpcode::FPMul32; break;
        case LLVMBC::Instruction::FDiv: opcode = IrOpcode::FDiv32; break;
        default: throw std::runtime_error("DXIL floating binary operation is not implemented: " + std::to_string(instruction.getOpcode()));
    }
    return builder.Emit(opcode, IrType::F32, {&left, &right});
}

IrValue& ScalarLowering::negate(const LLVMBC::UnaryOperator& instruction) {
    if (instruction.getOpcode() == LLVMBC::UnaryOperator::UnaryOps::FNeg && GetType(instruction) == IrType::F16) return builder.BitCastF16(builder.BitwiseXor(builder.BitCastU32FromF16(Get(*instruction.getOperand(0))), builder.Constant(0x8000)));
    if (instruction.getOpcode() != LLVMBC::UnaryOperator::UnaryOps::FNeg || GetType(instruction) != IrType::F32) throw std::runtime_error("DXIL scalar unary operation is not implemented");
    auto& value = Get(*instruction.getOperand(0));
    auto& bits = builder.BitwiseXor(builder.BitCastU32(value), builder.Constant(0x80000000));
    return builder.BitCastF32(bits);
}

IrValue& ScalarLowering::floatCast(const LLVMBC::CastInst& instruction) {
    auto* operand = instruction.getOperand(0);
    auto& value = Get(*operand);
    switch (instruction.getOpcode()) {
        case LLVMBC::Instruction::BitCast:
            if (GetType(instruction) == IrType::F32 && GetType(*operand) == IrType::F32) return value;
            if (GetType(instruction) == IrType::F32 && width(*operand) == 32) return builder.BitCastF32(value);
            if (GetType(*operand) == IrType::F32 && width(instruction) == 32) return builder.BitCastU32(value);
            throw std::runtime_error("DXIL floating bitcast requires exactly 32 bits");
        case LLVMBC::Instruction::UIToFP:
        case LLVMBC::Instruction::SIToFP: {
            auto bits = width(*operand);
            if (bits > 32 || GetType(instruction) != IrType::F32) throw std::runtime_error("DXIL integer-to-float conversion width is not implemented");
            bool isSigned = instruction.getOpcode() == LLVMBC::Instruction::SIToFP;
            auto& input = isSigned ? signedValue(value, bits) : unsignedValue(value);
            return builder.Emit(isSigned ? IrOpcode::ConvertF32S32 : IrOpcode::ConvertF32U32, IrType::F32, {&input});
        }
        case LLVMBC::Instruction::FPToUI:
        case LLVMBC::Instruction::FPToSI: {
            auto bits = width(instruction);
            if (bits > 32 || value.Type() != IrType::F32) throw std::runtime_error("DXIL float-to-integer conversion width is not implemented");
            auto opcode = instruction.getOpcode() == LLVMBC::Instruction::FPToSI ? IrOpcode::ConvertS32F32 : IrOpcode::ConvertU32F32;
            return narrow(builder.Emit(opcode, IrType::U32, {&value}), bits);
        }
        default: throw std::runtime_error("DXIL floating cast is not implemented");
    }
}
